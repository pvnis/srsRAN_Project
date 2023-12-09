#include "slice_repository.h"

using namespace srsran;

// load slice database
void slice_repository::load_slices(){
    // TODO: load slices from database or copy from gnb app config
    
    add_slice(std::make_unique<slice>(001, 01, 2, s_nssai_t{0,0}, 0));

    add_slice(std::make_unique<slice>(001, 02, 2, s_nssai_t{0,1}, 1));
}

void slice_repository::add_slice(std::unique_ptr<slice> s){
    // Add slice in repository.
    int slice_index = s->slice_index;

    slices.insert(slice_index, std::move(s));
}

void slice_repository::remove_slice(int index){
    slices.erase(index);
}
