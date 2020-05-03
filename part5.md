### Part 5 - Persistence to Disk  
#### Goal
Persisting records by saving the entire database to a file  
![](images/arch-part5.gif)  

#### Implementation  
Now we introduce `Pager` struct   
```c
typedef struct
{
    int file_id;
    uint32_t file_len;
    void * pages[TABLE_MAX_PAGES];
} Pager;
```
Then `Table` struct would be changed to hold `Pager` instead of all pages  
```c
typedef struct
{
    Pager * pager;
    uint32_t num_rows;
}Table;
```  
When we `INSERT` new row, we firstly decide *page num*, then we try to fetch that page. If that page is `NULL`, we create it by `malloc`, more importantly, we will read page-specific file content, i.e bytes ranging from `[page_num * PAGE_SIZE, (page_num + 1) * PAGE_SIZE)`  for cache, which is for  `SELECT` later.  
After close, we write all rows into db file, which of course will result in repeated writing for existing rows. 