select 
    max(an_name)
from 
    cast_info, 
    movie_info,
    aka_name,
    person_info
where 
    mi_movie_id=ci_movie_id 
    and ci_id = an_id
    and ci_person_id=pi_person_id 
    and an_name like '%Brad%'
    and mi_note like '%premiere%'
    and pi_info like '%6 ft%';