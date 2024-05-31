select 
    l_linestatus, avg(l_tax)
from
    lineitem
group by
    l_linestatus;