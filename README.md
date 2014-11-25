MPS Search Engine:
==================

This is a full text search engine written in C (with a little C++ thrown in). It was
originally based on the [WAIS](http://en.wikipedia.org/wiki/Wide_area_information_server)
search engine and served as the basis of the search engine that was used at Feedster.

After leaving Feedster I reworked all the code and used it for another project. I have
since replaced the search engine in that project with SOLR. 

So this is being released in the hope that it might be useful to others if they are 
interested in learning C, repurposing some of the code, etc...

There is very little documentation for it but I will add to it as time permits. 

You will need ICU and MeCab if you want to built it with CJK+T tokenization, but that
can be turned off if needed.


Commands:
---------

```
# Debug build with ICU and MeCab
./configure --with-build-type=debug --with-icu=/usr/local/icu --with-mecab=/usr/local/mecab --bindir=/usr/local/mps/bin --libdir=/usr/local/mps/lib
make
make install

# Debug build without ICU and MeCab (you will need to modify some defines in the code)
./configure --with-build-type=debug --bindir=/usr/local/mps/bin --libdir=/usr/local/mps/lib
make
make install

# Optimized build with ICU and MeCab
./configure --with-build-type=opt --with-icu=/usr/local/icu --with-mecab=/usr/local/mecab --bindir=/usr/local/mps/bin --libdir=/usr/local/mps/lib
```


Userful Links:
--------------

- [ICU](http://site.icu-project.org)
- [MeCab](http://mecab.googlecode.com/svn/trunk/mecab/doc/index.html)



