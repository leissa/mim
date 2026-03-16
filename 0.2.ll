


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
    br label %_2352

_2352:
    %_2356 = phi i64 [ 0, %_2318 ], [ %_2377, %_2353 ]
    %_2363 = icmp ult i64 %_2356, %_2320
    br i1 %_2363, label %_2353, label %_2354

_2354:
    ret i64 %_2356

_2353:
    %_2377 = add i64 1, %_2356
    br label %_2352

_2317:
    %_2350 = add i64 1, %_2320
    br label %_2315

}


