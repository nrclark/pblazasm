static void exch ( int a, int b ) {
    INST_t temp = Code[ a ] ;
    Code[ a ] = Code[ b ] ;
    Code[ b ] = temp ;
}

void quicksort ( int l, int r ) {
    int i = l - 1, j = r, p = l - 1, q = r, k ;
    uint32_t v = Code[ r ].code ;

    if ( r <= l )
        return ;
    for ( ;; ) {
        while ( Code[ ++i ].code < v )
            ;
        while ( v < Code[ --j ].code )
            if ( j == l )
                break;
        if ( i >= j )
            break;
        exch ( i, j );
        if ( Code[ i ].code == v ) {
            p++;
            exch ( p, i );
        }
        if ( v == Code[j].code ) {
            q--;
            exch ( j, q );
        }
    }
    exch ( i, r );
    j = i - 1;
    i = i + 1;
    for ( k = l; k < p; k++, j-- )
        exch ( k, j );
    for ( k = r - 1; k > q; k--, i++ )
        exch ( i, k );
    quicksort ( l, j );
    quicksort ( i, r );
}
