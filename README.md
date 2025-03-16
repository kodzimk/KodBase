<div align="center">
<pre>
KodBase
---------------------------------------------------
c minidatabase following rdbms rules
</pre>
</div>

# Short Explanation

This minidatabase are storing data on storage with folders.<br>
Each folder is one form or how we call tables.<br>
Using script .kb we can manipulate with database.<br>

## Installation

install this repo.

```sh
git clone (link of project)
make 
```

## Usage example
```sh
CREATE TABLE name_of_table(
    id INT,
    name VARCHAR(length_of_name),
    ....,
    ....
);
```

```sh
  SELECT item FROM table_name
  ADD SEGMENT name_of_segment TO table_name
```
