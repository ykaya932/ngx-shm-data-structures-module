# NGINX DATA STRUCTURES IN SHARED MEMORY MODULE

## Introduction

This is a module that I tried to understand the usage of builtin data structures and using them in the shared memory zone.

Requests are in the following format:

```bash
http://localhost/a_digit/a_number/
``` 

For each a_digit value rbtree is checked, if there isn't any node with a_digit value a new node is created. Then a_number is added to the hash table that is inside the newly created or existing rbtree node.

Since every request needs shared memory to be saved, all of the operations are done in shared memory zone.

## nginx.conf File

I have used the following lines in my conf file:

```
    server {
        listen       80;
        server_name  localhost;
        
        location = /tree{
            print-tree;
        }
        location ~ ^/\d/\d+$ {
            insert;
        }
    }
``` 

## Helpful Links

I have learned many things from [Emiller's Guide](https://www.evanmiller.org/nginx-modules-guide.html). It is very nice guide for beginners.

Also I have got help from [nginx-hello-world-module](https://github.com/perusio/nginx-hello-world-module). It was nice to see a whole module code.

And for data structures and shared memory, I often checked the [developement guide] written by nginx developers.