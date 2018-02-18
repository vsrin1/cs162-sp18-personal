# Notes for hw1
* tcsetpgrp should be called in parent process, and then the handler of SIGTTOU should be set to SIG_IGN