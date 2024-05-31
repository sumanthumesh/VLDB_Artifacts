SELECT MIN(mc_note) AS production_note,
       MIN(t_title) AS movie_title,
       MIN(t_production_year) AS movie_year
FROM company_type,
     info_type,
     movie_companies,
     movie_info_idx,
     title
WHERE ct_kind = 'production companies'
  AND it_info = 'top 250 rank'
  AND mc_note NOT LIKE '%(as Metro-Goldwyn-Mayer Pictures)%'
  AND (mc_note LIKE '%(co-production)%'
       OR mc_note LIKE '%(presents)%')
  AND ct_id = mc_company_type_id
  AND t_id = mc_movie_id
  AND t_id = mii_movie_id
  AND mc_movie_id = mii_movie_id
  AND it_id = mii_info_type_id;

