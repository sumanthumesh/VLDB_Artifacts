SELECT 
    MIN(t_title)
FROM 
    keyword,
    movie_info,
    movie_keyword,
    title
WHERE k_keyword LIKE '%sequel%'
  AND mi_info IN ('Sweden',
                  'Norway',
                  'Germany',
                  'Denmark',
                  'Swedish',
                  'Denish',
                  'Norwegian',
                  'German')
  AND t_production_year > 2005
  AND t_id = mi_movie_id
  AND t_id = mk_movie_id
  AND mk_movie_id = mi_movie_id
  AND k_id = mk_keyword_id;

