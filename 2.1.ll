


define  i64 @top(i64 %_2374)  {
top_2370:
    %_2376ret = call i64 @plzinline_2300(i64 %_2374)
    br label %_2371

_2371:
    %_2377 = phi i64 [ %_2376ret, %top_2370 ]
    ret i64 %_2377

}

define internal  i64 @plzinline_2300(i64 %_2302) alwaysinline  {
plzinline_2300:
    br label %_2306

_2306:
    %_2311 = phi i64 [ 0, %plzinline_2300 ], [ %_2341, %_2344 ]
    %_2321 = icmp ult i64 %_2311, %_2302
    br i1 %_2321, label %_2308, label %_2309

_2309:
    ret i64 %_2311

_2308:
    br label %_2342

_2342:
    %_2346 = phi i64 [ 0, %_2308 ], [ %_2367, %_2343 ]
    %_2353 = icmp ult i64 %_2346, %_2311
    br i1 %_2353, label %_2343, label %_2344

_2344:
    %_2341 = add i64 1, %_2311
    br label %_2306

_2343:
    %_2367 = add i64 1, %_2346
    br label %_2342

}


