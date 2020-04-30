### Part 2 - World's Simplest SQL Compiler and Virtual Machine
The "front-end" of sqlite is a SQL compiler that parses a string and outputs an internal representation called bytecode.  
This bytecode is passed to the virtual machine, which executes it.  
![](images/arch2.gif)