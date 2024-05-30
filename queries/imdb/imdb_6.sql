select
    max(ci_role_id),avg(ci_nr_order)
from
    cast_info,
    person_info,
    movie_info,
    title,
    char_name
where
    ci_person_id = pi_person_id
    and ci_movie_id = mi_movie_id
    and cn_imdb_index = t_imdb_index
    and ci_id = cn_id
    and pi_info like '%6 ft%'
    and mi_note not like '%split%'
    and t_production_year > 1960;