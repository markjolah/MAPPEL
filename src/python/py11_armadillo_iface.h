/** @file py11_armadillo_iface.h
 * @author Mark J. Olah (mjo\@cs.unm DOT edu)
 * @date 01-2018
 * @brief Definitions for the py11_armadillo namespace, numpy to armadillo conversions
 */

#ifndef _PY11_ARMADILLO_IFACE_H
#define _PY11_ARMADILLO_IFACE_H

#include <cstdint>
#include <sstream>
#include <armadillo>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "python_error.h"


namespace py11_armadillo 
{
using python_error::PythonError;

using IdxT = arma::uword; /**< A logical type for an IdxT integer index */    
template<class ElemT> using Vec = arma::Col<ElemT>;
template<class ElemT> using Mat = arma::Mat<ElemT>;
template<class ElemT> using Cube = arma::Cube<ElemT>;

constexpr static int ColumnMajorOrder = pybind11::array::f_style;
template<class ElemT, int Order=ColumnMajorOrder> using ArrayT = pybind11::array_t<ElemT,Order>;
using ArrayBoolT = pybind11::array_t<bool, pybind11::array::f_style>;
using ArrayDoubleT = pybind11::array_t<double, pybind11::array::f_style>;
using ArrayIdxT = pybind11::array_t<IdxT, pybind11::array::f_style>;
using ArrayUnsignedT = pybind11::array_t<IdxT, pybind11::array::f_style>;
using ArraySignedT = pybind11::array_t<int64_t, pybind11::array::f_style>;


template<class ElemT=double>
void checkMatShape(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    IdxT ndim = arr.ndim();
    if(ndim<1 || ndim>2) {
        std::ostringstream msg;
        msg<<"Cannot interpret as Mat, received Ndim: "<<arr.ndim()<< " Shape:[";
        for(int i=0;i<arr.ndim();i++) msg<<arr.shape(i)<<",";
        msg<<"\b]";
        throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
void checkCubeShape(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    IdxT ndim = arr.ndim();
    if(ndim<1 || ndim>3) {
        std::ostringstream msg;
        msg<<"Cannot interpret as Cube, received Ndim: "<<arr.ndim()<< " Shape:[";
        for(int i=0;i<arr.ndim();i++) msg<<arr.shape(i)<<",";
        msg<<"\b]";
        throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
void checkHpercubeShape(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    IdxT ndim = arr.ndim();
    if(ndim<1 || ndim>4) {
        std::ostringstream msg;
        msg<<"Cannot interpret as Hypercube, received Ndim: "<<arr.ndim()<< " Shape:[";
        for(int i=0;i<arr.ndim();i++) msg<<arr.shape(i)<<",";
        msg<<"\b]";
        throw PythonError("ConversionError",msg.str());
    }
}


template<class ElemT=double, int Layout=ColumnMajorOrder>
Vec<ElemT> asVec(ArrayT<ElemT, Layout> &arr)
{
    if(arr.size()==0) return {0};
    switch(arr.ndim()) { //copy_aux_mem=false, strict=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.size()), false, true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.size()), false, true};
        case 3:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0)), static_cast<IdxT>(arr.size()), false, true};
        case 4:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0,0)), static_cast<IdxT>(arr.size()), false, true};
        default:
            std::ostringstream msg;
            msg<<"Cannot convert to arma::Col. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double, int Layout=ColumnMajorOrder>
Vec<ElemT> copyVec(ArrayT<ElemT, Layout> &arr)
{
    if(arr.size()==0) return {0};
    switch(arr.ndim()) { //copy_aux_mem=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.size()), true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.size()), true};
        case 3:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0)), static_cast<IdxT>(arr.size()), true};
        case 4:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0,0)), static_cast<IdxT>(arr.size()), true};
        default:
            std::ostringstream msg;
            msg<<"Cannot copy to arma::Col. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
Mat<ElemT> asMat(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    switch(arr.ndim()) { //copy_aux_mem=false, strict=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.shape(0)), 1, false, true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), false, true};
        default:
            std::ostringstream msg;
            msg<<"Cannot convert to arma::Mat. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
Mat<ElemT> copyMat(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    switch(arr.ndim()) { //copy_aux_mem=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.shape(0)), 1, true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), true};
        default:
            std::ostringstream msg;
            msg<<"Cannot copy to arma::Mat. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
Cube<ElemT> asCube(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    switch(arr.ndim()) { //copy_aux_mem=false, strict=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.shape(0)), 1, 1, false, true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), 1, false, true};
        case 3:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), static_cast<IdxT>(arr.shape(2)), false, true};
        default:
            std::ostringstream msg;
            msg<<"Cannot convert to arma::Mat. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
Cube<ElemT> copyCube(ArrayT<ElemT, ColumnMajorOrder> &arr)
{
    switch(arr.ndim()) { //copy_aux_mem=true
        case 1:
            return {static_cast<ElemT*>(arr.mutable_data(0)), static_cast<IdxT>(arr.shape(0)), 1, 1, true};
        case 2:
            return {static_cast<ElemT*>(arr.mutable_data(0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), 1, true};
        case 3:
            return {static_cast<ElemT*>(arr.mutable_data(0,0,0)), static_cast<IdxT>(arr.shape(0)), static_cast<IdxT>(arr.shape(1)), static_cast<IdxT>(arr.shape(2)), true};
        default:
            std::ostringstream msg;
            msg<<"Cannot copy to arma::Cube. Got numpy ndarray dim="<<arr.ndim();
            throw PythonError("ConversionError",msg.str());
    }
}

template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeArray(IdxT rows)
{
    return ArrayT<ElemT, ColumnMajorOrder>(rows);
}


template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeArray(IdxT rows, IdxT cols)
{
    return ArrayT<ElemT, ColumnMajorOrder>({rows,cols});
}

template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeArray(IdxT rows, IdxT cols, IdxT slices)
{
    return ArrayT<ElemT, ColumnMajorOrder>({rows,cols,slices});
}

template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeSqueezedArray(IdxT rows)
{
    return ArrayT<ElemT, ColumnMajorOrder>(rows); //don't squeeze single element arrays as that would change type
}

template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeSqueezedArray(IdxT rows, IdxT cols)
{
    if(cols == 1) return ArrayT<ElemT, ColumnMajorOrder>({rows});  //squeeze out 2nd dim
    else           return ArrayT<ElemT, ColumnMajorOrder>({rows,cols});
}

template<class ElemT=double>
ArrayT<ElemT, ColumnMajorOrder> 
makeSqueezedArray(IdxT rows, IdxT cols, IdxT slices)
{
    if(slices == 1) return ArrayT<ElemT, ColumnMajorOrder>({rows,cols});  //squeeze out 3rd dim
    else            return ArrayT<ElemT, ColumnMajorOrder>({rows,cols,slices});
}


} /* namespace py11_armadillo */

#endif /*_PY11_ARMADILLO_IFACE_H */
