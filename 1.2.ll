


define  i64 @top(i64 %_2307)  {
top_2304:
    %_2309ret = call i64 @plzinline_2300(i64 %_2307)
    br label %_2305

_2305:
    %_2310 = phi i64 [ %_2309ret, %top_2304 ]
    ret i64 %_2310

}

define internal  i64 @plzinline_2300(i64 %_2302) alwaysinline  {
plzinline_2300:
    br label %_2315

_2315:
    %_2320 = phi i64 [ 0, %plzinline_2300 ], [ %_2350, %_2317 ]
    %_2330 = icmp ult i64 %_2320, %_2302
    br i1 %_2330, label %_2317, label %_2318

_2318:
    br label %_2362

_2362:
    %_2366 = phi i64 [ 0, %_2318 ], [ %_2387, %_2363 ]
    %_2373 = icmp ult i64 %_2366, %_2320
    br i1 %_2373, label %_2363, label %_2364

_2364:
    ret i64 %_2366

_2363:
    %_2387 = add i64 1, %_2366
    br label %_2362

_2317:
    %_2350 = add i64 1, %_2320
    br label %_2315

}


