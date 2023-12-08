#include "slice_repository.h"

using namespace srsran;

// load slice database
std::unique_ptr<slice_repository> load_slices(){
    std::unique_ptr<slice_repository> slices = std::make_unique<slice_repository>();
    // load slices from database
    std::unique_ptr<slice> slice = std::make_unique<slice>(001, 01, 2, {0,0}, 0);
    slices.add_slice(slice);
    return slices;
}

void slice_repository::add_slice(std::unique_ptr<slice> slice){
    // Add slice in repository.
    int slice_index = slice->slice_index;
    int plmn_id = slice->plmn_id;
    s_nssai_t nssai = slice->nssai;

    slices.insert(slice_index, std::move(slice));
}

void slice_repository::remove_slice(int index){
    slices.push(index);
}
