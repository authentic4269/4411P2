Mitchell Vogel, mjv58

Project Notes

My implementation requires an existing filesystem (ie, the one my mkfs creates) before the shell can be run. 
Indirect nodes are untested, and I've had some issues doing tests with larger file sizes so those may run into trouble.
Support for both of these cases does exist, however. 

There is an issue shell on my machine, although I don't know if it is unique to mine. I have to enter newlines to get the prompt again
after entering a command, and when using 'input'. This may cause issues with automated testing shell scripts.

My system allocates all of the root nodes direct blocks in mkfs. This may cause an issue if you try to make a tiny filesystem. I haven't
tested with less than 64 blocks. 

I learned a lot doing this project, and although it was very stressful doing it all by myself, I enjoyed it. Steven Yeh, my 
partner, gave up contributing during the course of implementation.  
