select 
    max(at_title)
from 
    aka_title, 
    cast_info, 
    person_info,
    name 
where 
    at_movie_id=ci_movie_id 
    and ci_id = n_id
    and ci_person_id=pi_person_id 
    and n_name like '%Mike%'
    and pi_info like '%5 ft%';