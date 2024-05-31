select
    max(n_name), min(t_production_year)
from
    cast_info,
    movie_info,
    person_info,
    name,
    title
where
    ci_movie_id = mi_movie_id
    and ci_person_id = pi_person_id
    and ci_id = n_id
    and ci_note like '%archive%'
    and mi_note like '%original%'
    and t_id = ci_id;