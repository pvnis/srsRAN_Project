#include "slice_repository.h"

using namespace srsran;

// load slice database
slice_repository load_slices(){
    slice_repository slices;
    // load slices from database
    slice slice;
    slice.slice_index = 0;
    slice.plmn_id = 00101;
    slice.nssai = {0,0};
    slices.add_slice(slice);
    return slices;
}

void slice_repository::add_slice(std::unique_ptr<slice> slice){
    // Add slice in repository.
    slice_index = slice->slice_index;
    int plmn_id = slice->plmn_id;
    s_nssai_t nssai = slice->nssai;

    slice.insert(slice_index, std::move(slice));
}

void slice_repository::remove_slice(slice_index index){
    slices.push(index);
}

slice* slice_repository::find(slice_index index){
    // TODO
}

const slice* slice_repository::find(slice_index index){
    // TODO
}