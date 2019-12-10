# Final Project

Group name: os_fp-teamlast

group members : Marcus Cantu, Jordan Fitzgerald, Jacob Schmidt



compile instructions:
make  
 

## Working/Not Working
Everything should be working as expected.

## Driver 6
We chose file type (txt, gif, png, etc) which must be three letters so docx would be an invalid type. For our second one we chose file owner as in "c2" which must be no more than 2 chars long. 


---

**grading is out of 100 points**
* create file (imples r/w in dm, pm r/w, getfree, and returnblock) (10 points):  yes, but
  * memory vulernablity in the bitvector...  really should take points.
* openfile (and close 3points of) opentable, etc ignoring unlock/lock (10 points): yes
* readfile (10 points): yes
  * very nice method.  
* writefile (10 points): yes
* seekfile (5 points): yes
* appendfile ( 5 points, since just call see to end and write): yes
* createdir (10 points) => implies create,open work as well: yes
* lock and unlock (5 each)  looped into open/close as well: yes
* rename (5 points): yes
* deletefile (5 points), remember lock again: yes
* deletedir (5 points), remember empty: yes
* attributes read/set (5 points each): yes
  * checks if there is a value in the getAtt.
other notes:
  * disk is
    * driver 1: overall good.  gibberish in the bitvector, so wasn't set to blanks before the read
       * no default attributes for the files.  get/read needs to know how to deal with it.
    * driver 2: bv on A now looks to have a file a inode listed now.  may not effect the run, but memory issues. parition c has file b listed..  Otherwise the filesystem appears to be working. deletes and renames appear to work.
    * driver 3: other then gibberish at the end of bv block, file system is consistent and directories appear to work.
    * driver 4: partitions look correct and sub directories are working.
    * driver 5: edge test worked.  disk  is consistent
    * driver 6: attrib appear to work.  visiable on disk.
  * other
    * code looks
       * overall looks good.  hard to read in some places and better comments and block comments for helper functions would have really been useful.
       * added defaults to buffer in partitions and gibberish goes away.  This by the way is called heartbleed vunlerablity in web servers.  same problem.
  * group eval:  individual grades will be emailed.

