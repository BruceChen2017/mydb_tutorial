### Part 6 - The Cursor Abstraction  
Now we create `Cursor` abstraction to represent a location in the table, which hides details of table (how the table is stored) for users. `Cursor` points to start byte of a row in the table, which is simply indicated by `Cursor->row_num`. Row pointed to by `Cursor` may be empty row.  
Things you might want to do with cursors:
- Create a cursor at the beginning of the table
- Create a cursor at the end of the table
- Access the row the cursor is pointing to
- Advance the cursor to the next row