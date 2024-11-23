# ls(1) for CS631

This project was really fun overall but there were many edge cases that need to
be dealt with. Also, figuring out how fts worked took some time.

I first realized that the fts functions would fail on the recursion if the
paths got too long. So, I had to remove FTS_NOCHDIR. This breaks symlink access
so I realized you need to get the parent's accpath and append the name of the
file. Still doesn't seem like a clean solution to me, but it seems to work.
