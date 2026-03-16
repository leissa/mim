


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
    br label %_2379

_2379:
    %_2383 = phi i64 [ 0, %_2354 ], [ %_2404, %_2380 ]
    %_2390 = icmp ult i64 %_2383, %_2356
    br i1 %_2390, label %_2380, label %_2381

_2381:
    br label %_2406

_2406:
    %_2410 = phi i64 [ 0, %_2381 ], [ %_2431, %_2407 ]
    %_2417 = icmp ult i64 %_2410, %_2383
    br i1 %_2417, label %_2407, label %_2408

_2408:
    ret i64 %_2410

_2407:
    %_2431 = add i64 1, %_2410
    br label %_2406

_2380:
    %_2404 = add i64 1, %_2383
    br label %_2379

_2353:
    %_2377 = add i64 1, %_2356
    br label %_2352

_2317:
    %_2350 = add i64 1, %_2320
    br label %_2315

}


