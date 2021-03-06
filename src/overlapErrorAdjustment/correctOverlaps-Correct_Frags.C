
/******************************************************************************
 *
 *  This file is part of canu, a software program that assembles whole-genome
 *  sequencing reads into contigs.
 *
 *  This software is based on:
 *    'Celera Assembler' (http://wgs-assembler.sourceforge.net)
 *    the 'kmer package' (http://kmer.sourceforge.net)
 *  both originally distributed by Applera Corporation under the GNU General
 *  Public License, version 2.
 *
 *  Canu branched from Celera Assembler at its revision 4587.
 *  Canu branched from the kmer project at its revision 1994.
 *
 *  Modifications by:
 *
 *    Brian P. Walenz from 2015-MAY-20 to 2015-JUN-03
 *      are Copyright 2015 Battelle National Biodefense Institute, and
 *      are subject to the BSD 3-Clause License
 *
 *    Brian P. Walenz beginning on 2016-MAY-02
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *    Sergey Nurk beginning on 2019-AUG-23
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *    Sergey Koren beginning on 2019-OCT-01
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *  File 'README.licenses' in the root directory of this distribution contains
 *  full conditions and disclaimers for each license.
 */

#include "correctOverlaps.H"
#include "correctionOutput.H"


//  Shouldn't be global.
char  filter[256];


void
correctRead(uint32 curID,
            char *fseq, uint32 &fseqLen, Adjust_t *fadj, uint32 &fadjLen,
            const char *oseq, uint32  oseqLen,
            Correction_Output_t  *C,
            uint64               &Cpos,
            uint64                Clen,
            uint64               *changes) {

#if DO_NO_CORRECTIONS
  //  for testing if the adjustments are screwed up.  yup.
  strcpy(fseq, oseq);
  fseqLen += oseqLen;
  return;
#endif

  //fprintf(stderr, "Correcting read %u\n", curID);

  //  Find the correct corrections.

  while ((Cpos < Clen) && (C[Cpos].readID < curID)) {
    //fprintf(stderr, "SKIP Cpos=%u Clen=%u for read %u, want read %u\n", Cpos, Clen, C[Cpos].readID, curID);
    Cpos++;
  }

  //  Skip any IDENT message.

  assert(C[Cpos].type == IDENT);

  //G.reads[G.readsLen].keep_left  = C[Cpos].keep_left;
  //G.reads[G.readsLen].keep_right = C[Cpos].keep_right;

  Cpos++;
  assert(Cpos <= Clen);

  //fprintf(stderr, "Start at Cpos=%d position=%d type=%d id=%d\n", Cpos, C[Cpos].pos, C[Cpos].type, C[Cpos].readID);

  int32   adjVal = 0;

  for (uint32 i = 0; i < oseqLen; ) {

    //  No more corrections OR no more corrections for this read OR no correction at position -- just copy base
    if (Cpos == Clen || C[Cpos].readID != curID || i < C[Cpos].pos) {
      //fprintf(stderr, "Introducing IDENT '%c' read=%u i=%u \n", filter[oseq[i]], C[Cpos].readID, i);
      fseq[fseqLen++] = filter[oseq[i++]];
      continue;
    }

    assert(Cpos < Clen);
    assert(i == C[Cpos].pos);

    if (changes)
      changes[C[Cpos].type]++;

    switch (C[Cpos].type) {
      case DELETE:  //  Delete base
        //fprintf(stderr, "DELETE %u pos %u adjust %d\n", fadjLen, i+1, adjVal-1);
        //fprintf(stderr, "Introducing DELETION read=%u i=%u \n", C[Cpos].readID, i);
        fadj[fadjLen].adjpos = i + 1;
        fadj[fadjLen].adjust = --adjVal;
        fadjLen++;
        i++;
        break;

      case A_SUBST:
      case C_SUBST:
      case G_SUBST:
      case T_SUBST:
        //fprintf(stderr, "Introducing SUBST '%c' -> '%c' read=%u i=%u \n", filter[oseq[i]], VoteChar(C[Cpos].type), C[Cpos].readID, i);
        fseq[fseqLen++] = VoteChar(C[Cpos].type);
        i++;
        break;

      case A_INSERT:
      case C_INSERT:
      case G_INSERT:
      case T_INSERT:
        //fprintf(stderr, "Introducing INSERTION '%c' read=%u i=%u \n", VoteChar(C[Cpos].type), C[Cpos].readID, i);
        fseq[fseqLen++] = VoteChar(C[Cpos].type);

        fadj[fadjLen].adjpos = i + 1;
        fadj[fadjLen].adjust = ++adjVal;
        fadjLen++;
        break;

      default:
        fprintf (stderr, "ERROR:  Illegal vote type\n");
        break;
    }

    Cpos++;
  }

  //  Terminate the sequence.
  fseq[fseqLen] = 0;

  //fprintf(stdout, ">%u\n%s\n", curID, fseq);
}









//  Open and read corrections from  Correct_File_Path  and
//  apply them to sequences in  Frag .

//  Load reads from seqStore, and apply corrections.

void
Correct_Frags(coParameters *G,
              sqStore      *seqStore) {

  //  The original converted to lowercase, and made non-acgt be 'a'.

  for (uint32 i=0; i<256; i++)
    filter[i] = 'a';

  filter['A'] = filter['a'] = 'a';
  filter['C'] = filter['c'] = 'c';
  filter['G'] = filter['g'] = 'g';
  filter['T'] = filter['t'] = 't';

  //  Open the corrections, as an array.

  memoryMappedFile     *Cfile = new memoryMappedFile(G->correctionsName);
  Correction_Output_t  *C     = (Correction_Output_t *)Cfile->get();
  uint64                Cpos  = 0;
  uint64                Clen  = Cfile->length() / sizeof(Correction_Output_t);

  uint64     firstRecord   = 0;
  uint64     currentRecord = 0;

  fprintf(stderr, "Reading " F_U64 " corrections from '%s'.\n", Clen, G->correctionsName);

  //  Count the number of bases, so we can do two gigantic allocations for bases and adjustments.
  //  Adjustments are always less than the number of corrections; we could also count exactly.

  G->basesLen   = 0;

  for (uint32 curID = G->bgnID; curID <= G->endID; curID++) {
    sqRead *read = seqStore->sqStore_getRead(curID);

    G->basesLen += read->sqRead_sequenceLength() + 1;
  }

  uint64 del_cnt = 0;
  uint64 ins_cnt = 0;
  for (uint64 c = 0; c < Clen; c++) {
    switch (C[c].type) {
      case DELETE:
        del_cnt++;
        break;
      case A_INSERT:
      case C_INSERT:
      case G_INSERT:
      case T_INSERT:
        ins_cnt++;
        break;
      default: {}
    }
  }
  G->basesLen += ins_cnt; // allow extra space for insertions in case reads get longer
  G->adjustsLen = ins_cnt + del_cnt;

  fprintf(stderr, "Correcting " F_U64 " bases with " F_U64 " indel adjustments.\n", G->basesLen, G->adjustsLen);

  fprintf(stderr, "--Allocate " F_U64 " + " F_U64 " + " F_U64 " MB for bases, adjusts and reads.\n",
          (sizeof(char)        * (uint64)(G->basesLen))             / 1048576,   //  MacOS GCC 4.9.4 can't decide if these three
          (sizeof(Adjust_t)    * (uint64)(G->adjustsLen))           / 1048576,   //  values are %u, %lu or %llu.  We force cast
          (sizeof(Frag_Info_t) * (uint64)(G->endID - G->bgnID + 1)) / 1048576);  //  them to be uint64.

  G->bases        = new char          [G->basesLen];
  G->adjusts      = new Adjust_t      [G->adjustsLen];
  G->reads        = new Frag_Info_t   [G->endID - G->bgnID + 1];
  G->readsLen     = 0;

  G->basesLen   = 0;
  G->adjustsLen = 0;

  uint64   changes[12] = {0};

  //  Load reads and apply corrections for each one.

  sqReadData *readData = new sqReadData;

  for (uint32 curID=G->bgnID; curID<=G->endID; curID++) {
    sqRead *read       = seqStore->sqStore_getRead(curID);

    seqStore->sqStore_loadReadData(read, readData);

    uint32  readLength = read->sqRead_sequenceLength();
    char   *readBases  = readData->sqReadData_getSequence();

    //  Save pointers to the bases and adjustments.

    G->reads[G->readsLen].bases       = G->bases   + G->basesLen;
    G->reads[G->readsLen].basesLen    = 0;
    G->reads[G->readsLen].adjusts     = G->adjusts + G->adjustsLen;
    G->reads[G->readsLen].adjustsLen  = 0;

    //  Find the correct corrections.

    while ((Cpos < Clen) && (C[Cpos].readID < curID))
      Cpos++;

    //  We should be at the IDENT message.

    if (C[Cpos].type != IDENT) {
      fprintf(stderr, "ERROR: didn't find IDENT at Cpos=" F_U64 " for read " F_U32 "\n", Cpos, curID);
      fprintf(stderr, "       C[Cpos] = keep_left=%u keep_right=%u type=%u pos=%u readID=%u\n",
              C[Cpos].keep_left,
              C[Cpos].keep_right,
              C[Cpos].type,
              C[Cpos].pos,
              C[Cpos].readID);
    }
    assert(C[Cpos].type == IDENT);

    G->reads[G->readsLen].keep_left  = C[Cpos].keep_left;
    G->reads[G->readsLen].keep_right = C[Cpos].keep_right;

    //Cpos++;

    //  Now do the corrections.

    correctRead(curID,
                G->reads[G->readsLen].bases,
                G->reads[G->readsLen].basesLen,
                G->reads[G->readsLen].adjusts,
                G->reads[G->readsLen].adjustsLen,
                readData->sqReadData_getSequence(),
                read->sqRead_sequenceLength(),
                C,
                Cpos,
                Clen,
                changes);

    //  Update the lengths in the globals.

    G->basesLen   += G->reads[G->readsLen].basesLen   + 1;
    G->adjustsLen += G->reads[G->readsLen].adjustsLen;
    G->readsLen   += 1;
  }

  delete readData;
  delete Cfile;

  fprintf(stderr, "Corrected " F_U64 " bases with " F_U64 " substitutions, " F_U64 " deletions and " F_U64 " insertions.\n",
          G->basesLen,
          changes[A_SUBST] + changes[C_SUBST] + changes[G_SUBST] + changes[T_SUBST],
          changes[DELETE],
          changes[A_INSERT] + changes[C_INSERT] + changes[G_INSERT] + changes[T_INSERT]);
}
