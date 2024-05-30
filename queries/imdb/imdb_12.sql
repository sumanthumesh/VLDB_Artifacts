select
    max(n_name)
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
    and ci_note like '%uncredited%'
    and mi_note like '%dialogue%'
    and cn_name like '%Smith%';