This project was really fun overall but there were many edge cases that need to
be dealt with. Also, figuring out how fts worked took some time.

I first realized that the fts functions would fail on the recursion if the
paths got too long. So, I had to remove FTS_NOCHDIR. This breaks symlink access
so I realized you need to get the parent's accpath and append the name of the
file.

I noticed a diff in output for -lairtus. I realized that the difference
was normal because the program itself was in the directory I was listing.
Running the executable updates the atim of the file. When I copied the /bin/ls
utility into the same directory, I observed a similar result.

I made other test cases like ls -sh and I noticed that /bin/ls prints the file
size in human readable format in that case. So I had to fix the output in those
cases. This is shown in the git history.

Note:
I don't know why but my VM was out of sync by a week for some reason. I made
the first three commits a week later than shown. After reboot it fixed itself.
