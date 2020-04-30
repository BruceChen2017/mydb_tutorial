### Part 1 - Introduction and Setting up the REPL
#### Sqlite
**Sqlite Architecture**:  
![sqlite architecture](images/arch1.gif) 

The *front-end* consists of the:
- tokenizer
- parser
- code generator 

The input to the front-end is a SQL query. the output is sqlite virtual machine bytecode (essentially a compiled program that can operate on the database)  

The *back-end* consists of the:  
- virtual machine
- B-tree
- pager
- OS interface