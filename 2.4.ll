


define  i64 @top(i64 %_2455)  {
top_2451:
    %_2457ret = call i64 @plzinline_2300(i64 %_2455)
    br label %_2452

_2452:
    %_2458 = phi i64 [ %_2457ret, %top_2451 ]
    ret i64 %_2458

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
    %_2346 = phi i64 [ 0, %_2308 ], [ %_2367, %_2370 ]
    %_2353 = icmp ult i64 %_2346, %_2311
    br i1 %_2353, label %_2343, label %_2344

_2344:
    %_2341 = add i64 1, %_2311
    br label %_2306

_2343:
    br label %_2368

_2368:
    %_2372 = phi i64 [ 0, %_2343 ], [ %_2393, %_2396 ]
    %_2379 = icmp ult i64 %_2372, %_2346
    br i1 %_2379, label %_2369, label %_2370

_2370:
    %_2367 = add i64 1, %_2346
    br label %_2342

_2369:
    br label %_2394

_2394:
    %_2398 = phi i64 [ 0, %_2369 ], [ %_2419, %_2422 ]
    %_2405 = icmp ult i64 %_2398, %_2372
    br i1 %_2405, label %_2395, label %_2396

_2396:
    %_2393 = add i64 1, %_2372
    br label %_2368

_2395:
    br label %_2420

_2420:
    %_2424 = phi i64 [ 0, %_2395 ], [ %_2445, %_2421 ]
    %_2431 = icmp ult i64 %_2424, %_2398
    br i1 %_2431, label %_2421, label %_2422

_2422:
    %_2419 = add i64 1, %_2398
    br label %_2394

_2421:
    %_2445 = add i64 1, %_2424
    br label %_2420

}


