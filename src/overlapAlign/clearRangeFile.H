
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
 *    Brian P. Walenz beginning on 2019-JUN-07
 *      are a 'United States Government Work', and
 *      are released in the public domain
 *
 *  File 'README.licenses' in the root directory of this distribution contains
 *  full conditions and disclaimers for each license.
 */

#ifndef CLEAR_RANGE_FILE_H
#define CLEAR_RANGE_FILE_H

#include "AS_global.H"
#include "clearRangeFile.H"

//  Store a set of clear ranges on disk.

class clearRangeFile {
public:
  clearRangeFile(char *filename);
  ~clearRangeFile();

private:
  void      setFilename(char *filename);

public:
  void      loadData(char *filename=NULL);
  void      saveData(char *filename=NULL, bool force=false);

private:
  void      reallocData(uint64 newmax, bool exact=false);

public:
  uint32    bgn(uint32 id)      { return(_bgn[id]); };
  uint32    end(uint32 id)      { return(_end[id]); };
  bool      valid(uint32 id)    { return((_flags[id] & 0x01) != 0); };
  bool      deleted(uint32 id)  { return((_flags[id] & 0x02) != 0); };

  void      setClearRange(uint32 id, uint32 bgn, uint32 end, bool deleted) {
    _modified = true;

    reallocData(id);

    _bgn[id]    = bgn;
    _end[id]    = end;
    _flags[id]  = 0x01;     //  Set valid, clear deleted.

    if (deleted)            //  Valid, but deleted.
      _flags[id] |= 0x02;

    _minSet = min(_minSet, id);
    _maxSet = max(id, _maxSet);
  };

private:
  char     _filename[FILENAME_MAX+1];

  bool     _modified;

  uint32   _minSet;   //  _maxSet is a misnomer; it's actually one past the last valid value.
  uint32   _maxSet;

  uint64   _maxAlloc;

  uint32  *_bgn;
  uint32  *_end;
  uint8   *_flags;
};

#endif // CLEAR_RANGE_FILE_H
