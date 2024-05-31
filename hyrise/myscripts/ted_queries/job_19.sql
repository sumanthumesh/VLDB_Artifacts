SELECT 
    MIN(t_title) AS movie_title 
FROM 
    company_name,
    keyword,
    movie_companies,
    movie_keyword,
    title
WHERE con_country_code ='[de]'
  AND k_keyword ='character-name-in-title'
  AND con_id = mc_company_id
  AND mc_movie_id = t_id
  AND t_id = mk_movie_id
  AND mk_keyword_id = k_id
  AND mc_movie_id = mk_movie_id;

