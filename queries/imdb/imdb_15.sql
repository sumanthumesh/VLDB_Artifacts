select
    max(n_name)
from
    cast_info,
    movie_info,
    person_info,
    name
where
    ci_movie_id = mi_movie_id
    and ci_person_id = pi_person_id
    and ci_note like '%archive%'
    and mi_note like '%original%'
    and pi_info like '%rapper%'
    and ci_id = n_id;