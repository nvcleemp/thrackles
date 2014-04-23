Thrackles
=========

This repository groups several programs and code snippets to work with thrackles.

A thrackle is an embedded graph such that the following conditions are met:

* no edge intersects itself;
* adjacent edges do not intersect (except for the common vertex);
* non-adjacent edges intersect exactly once.

For more information about thrackles we refer to http://www.thrackle.org.


File format
-----------

Most programs in this repository work with the file format known as `thrackle_code`.
This is a simple binary format that encodes the thrackle, i.e., the combinatorial embedding of the vertices and the intersections.
In the next paragraphs you find an overview of this file format.

If there are n vertices and i intersections, we label the vertices with the numbers 1, 2, ..., n, and the intersections with the numbers n+1, n+2, ..., n+i.

The file starts with the header.
This header is always `>>thrackle_code<<`.
After the header a sequence of numbers follows.
If the first number is 0, then all other numbers occupy two bytes.
If there is no zero, than all numbers occupy a single byte.
The first number is the number of vertices, the second number is the number of intersections.
After that we gives the cyclic order of vertices and intersections around each vertex (in clockwise order) and close each list with a zero.

The five star is a thrackle embedding of the 5-cycle.
In `thrackle_code` this would for example be:

    5 5 6 10 0 7 8 0 9 10 0 6 7 0 8 9 0 1 4 7 10 0 2 8 6 4 0 2 5 9 7 0 3 10 8 5 0 1 6 9 3 0

The thrackle embedding for the 3-cycle would be:

    3 0 2 3 0 1 3 0 1 2 0

