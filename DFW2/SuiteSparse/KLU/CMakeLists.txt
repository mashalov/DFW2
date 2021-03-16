cmake_minimum_required(VERSION 3.16)
project(KLULibrary)
set(CMAKE_C_STANDARD 11)
set(SOURCES Source/klu_analyze_given.c 
            Source/klu_analyze.c 
            Source/klu_defaults.c 
            Source/klu_diagnostics.c 
            Source/klu_dump.c 
            Source/klu_extract.c 
            Source/klu_factor.c 
            Source/klu_free_numeric.c 
            Source/klu_free_symbolic.c 
            Source/klu_kernel.c 
            Source/klu_memory.c 
            Source/klu_refactor.c 
            Source/klu_scale.c 
            Source/klu_solve.c 
            Source/klu_sort.c 
            Source/klu_tsolve.c
            Source/klu.c

            ../AMD/Source/amd_1.c
            ../AMD/Source/amd_2.c
            ../AMD/Source/amd_aat.c
            ../AMD/Source/amd_control.c
            ../AMD/Source/amd_defaults.c
            ../AMD/Source/amd_dump.c
            ../AMD/Source/amd_global.c
            ../AMD/Source/amd_info.c
            ../AMD/Source/amd_order.c
            ../AMD/Source/amd_post_tree.c
            ../AMD/Source/amd_postorder.c
            ../AMD/Source/amd_preprocess.c
            ../AMD/Source/amd_valid.c
            ../AMD/Source/amd.f
            ../AMD/Source/amdbar.f

            ../BTF/Source/btf_maxtrans.c
            ../BTF/Source/btf_order.c
            ../BTF/Source/btf_strongcomp.c

            ../COLAMD/Source/colamd_global.c
            ../COLAMD/Source/colamd.c

            ../CSparse/Source/cs_add.c
            ../CSparse/Source/cs_amd.c
            ../CSparse/Source/cs_chol.c
            ../CSparse/Source/cs_cholsol.c
            ../CSparse/Source/cs_compress.c
            ../CSparse/Source/cs_counts.c
            ../CSparse/Source/cs_cumsum.c
            ../CSparse/Source/cs_dfs.c
            ../CSparse/Source/cs_dmperm.c
            ../CSparse/Source/cs_droptol.c
            ../CSparse/Source/cs_dropzeros.c
            ../CSparse/Source/cs_dupl.c
            ../CSparse/Source/cs_entry.c
            ../CSparse/Source/cs_ereach.c
            ../CSparse/Source/cs_etree.c
            ../CSparse/Source/cs_fkeep.c
            ../CSparse/Source/cs_gaxpy.c
            ../CSparse/Source/cs_happly.c
            ../CSparse/Source/cs_house.c
            ../CSparse/Source/cs_ipvec.c
            ../CSparse/Source/cs_leaf.c
            ../CSparse/Source/cs_load.c
            ../CSparse/Source/cs_lsolve.c
            ../CSparse/Source/cs_ltsolve.c
            ../CSparse/Source/cs_lu.c
            ../CSparse/Source/cs_lusol.c
            ../CSparse/Source/cs_malloc.c
            ../CSparse/Source/cs_maxtrans.c
            ../CSparse/Source/cs_multiply.c
            ../CSparse/Source/cs_norm.c
            ../CSparse/Source/cs_permute.c
            ../CSparse/Source/cs_pinv.c
            ../CSparse/Source/cs_post.c
            ../CSparse/Source/cs_print.c
            ../CSparse/Source/cs_pvec.c
            ../CSparse/Source/cs_qr.c
            ../CSparse/Source/cs_qrsol.c
            ../CSparse/Source/cs_randperm.c
            ../CSparse/Source/cs_reach.c
            ../CSparse/Source/cs_scatter.c
            ../CSparse/Source/cs_scc.c
            ../CSparse/Source/cs_schol.c
            ../CSparse/Source/cs_spsolve.c
            ../CSparse/Source/cs_sqr.c
            ../CSparse/Source/cs_symperm.c
            ../CSparse/Source/cs_tdfs.c
            ../CSparse/Source/cs_transpose.c
            ../CSparse/Source/cs_updown.c
            ../CSparse/Source/cs_usolve.c
            ../CSparse/Source/cs_util.c
            ../CSparse/Source/cs_utsolve.c

            ../SuiteSparse_config/SuiteSparse_config.c
            )

set(ZSOURCES Source/Complex/klu_factor.c
             Source/Complex/klu_free_numeric.c
             Source/Complex/klu_kernel.c
             Source/Complex/klu_refactor.c
             Source/Complex/klu_scale.c
             Source/Complex/klu_solve.c
             Source/Complex/klu_sort.c
             Source/Complex/klu_tsolve.c
             Source/Complex/klu.c
             ../SuiteSparse_config/SuiteSparse_config.c
             )            

set(KLUINCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/Include" 
             "${CMAKE_CURRENT_SOURCE_DIR}/../AMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/../COLAMD/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/../BTF/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/../CSparse/Include"
             "${CMAKE_CURRENT_SOURCE_DIR}/../SuiteSparse_config/")


add_library(klu ${SOURCES})
add_library(kluz ${ZSOURCES})
target_compile_definitions(kluz PRIVATE COMPLEX)
target_include_directories(klu PRIVATE ${KLUINCLUDES})
target_include_directories(kluz PRIVATE ${KLUINCLUDES})