This repo is a modified fork of the original canu repo.

## Important note

__Note that the default branch of this repo is not `master` but `v1.9_wdl_patch`.__

`v1.9_wdl_patch` is based on the v1.9 release of canu, and contains

  * firstly re-formatting of the perl script `canu.wdl` and perl modules
  * non-trivial adaptation of the perl code for friendliness to WDL and cloud (but does not contain any changes to the functional/C code)
  * cherry-picked the following two commits&mdash;**pushed after the v1.9 release**&mdash;from the original/official canu repo fixing bugs in the meryl module:
    
    ```git
    commit 82f608b4474fc8bd2d1416a1770f11c4e5348f9a
    Author: Brian P. Walenz <thebri@gmail.com>
    Date:   Wed Nov 13 16:17:50 2019 -0500

        Fix issue computing cutoff threshold when lots of noise kmers are present.  Issue #1538.

    commit 0fbbd0f6f070a01f2890540e44f02311dabd1e69
    Author: Brian P. Walenz <thebri@gmail.com>
    Date:   Wed Nov 13 16:17:13 2019 -0500

        Allow running with empty merylDB filename, for testing the threshold computation.  Issue #1538.

    ```
