select 
    max(at_production_year) 
from 
    aka_title, 
    cast_info, 
    person_info 
where 
    at_movie_id=ci_movie_id 
    and ci_person_id=pi_person_id 
    and pi_info like '%5 ft%';