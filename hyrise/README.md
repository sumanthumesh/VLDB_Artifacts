Install dependencies
* `bash install_dependencies.sh`

To compile use
* `mkdir build && cd build`
* `cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_TYPE=Release ..`
* `make -j`

To run a TPC-H query
* `python tpch_one_click.py <query> <scaling factor> ../build`

Or to run a imdb query
* `python tpch_one_click.py <query> 1 ../build`

The results from each query run will be placed in `q_<query>_<scaling factor>_<pid>`

`isolate_<query>.trace` is the memory trace where all accesses are to DRAM

To generate a mapping use the `optimizer.py` script and follow the instructions contained within
Similarly to generate a new remapped trace use the `remap.py` or `remap_job.py` script files for TPC-H and IMDB queries respectively