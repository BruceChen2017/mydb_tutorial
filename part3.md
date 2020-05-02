### Part 3 - An In-Memory, Append-Only, Single-Table Database  
#### Goal 
- support two operations: inserting a row and printing all rows
- reside only in memory (no persistence to disk)
- support a single, hard-coded table  
#### Implementation  
- Store rows in blocks of memory called pages
- Each page stores as many rows as it can fit
- Rows are serialized into a compact representation with each page
- Pages are only allocated as needed
- Keep a fixed-size array of pointers to pages
#### Table Scheme(example)
|  column    |   type   |
| ---- | ---- | 
|  id    |   int   |
|  username   |   varchar(32)   |
|  email    |  varchar(255)    |