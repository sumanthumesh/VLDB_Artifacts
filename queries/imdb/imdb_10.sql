select
    max(n_name),ci_person_role_id
from
    cast_info,
    movie_info,
    person_info,
    name,
    char_name
where
    ci_movie_id = mi_movie_id
    and ci_person_id = pi_person_id
    and ci_id = n_id
    and cn_id = ci_id
    and ci_note like '%archive%'
    and mi_note like '%original%'
    and cn_name like '%Smith%'
group by
    ci_person_role_id;