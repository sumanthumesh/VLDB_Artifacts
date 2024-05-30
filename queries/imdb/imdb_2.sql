select 
    max(cn_name)
from 
    cast_info,
    person_info,
    char_name,
    name
where 
    ci_person_id = pi_person_id
    and cn_id = ci_id
    and n_id = cn_id
    and n_name like '%Smith%'
    and cn_name like '%David%'
    and pi_info like '%guitar%'
    and ci_note like '%short%';