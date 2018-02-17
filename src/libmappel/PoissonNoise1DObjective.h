/** @file PoissonNoise1DObjective.h
 * @author Mark J. Olah (mjo\@cs.unm.edu)
 * @date 2014-2018
 * @brief The class declaration and inline and templated functions for PoissonNoise1DObjective.
 */

#ifndef _MAPPEL_POISSONNOISE1DOBJECTIVE_H
#define _MAPPEL_POISSONNOISE1DOBJECTIVE_H

#include "ImageFormat1DBase.h"
#include "PoissonNoise2DObjective.h"
#include "estimator.h"

namespace mappel {

/** @brief A base class for 1D objectives with Poisson read noise.
 * This objective function and its subclasses are for models where the only source of noise is the "shot" or 
 * "counting" or Poisson noise inherent to a discrete capture of phontons given a certain mean rate of 
 * incidence on each pixel.
 * 
 */
class PoissonNoise1DObjective : public virtual ImageFormat1DBase {
public:
    static const std::vector<std::string> estimator_names;
    using ModelDataT = ImageT; /**< Objective function data type: 1D double precision image, gain-corrected to approximate photons counts */
    using ModelDataStackT = ImageStackT; /**< Objective function data stack type: 1D double precision image stack, of images gain-corrected to approximate photons counts */
};

namespace methods {
/** @brief Simulate an image at a given theta stencil, by generating Poisson noise
    * Enabled for PoissonNoise1DObjective
    * @param[in] model Model object
    * @param[in] s The stencil computed at theta.
    * @param[in,out] rng A random number generator
    * @returns A simulated image at theta under the noise model.
    */
template<class Model, class rng_t>
ReturnIfSubclassT<ModelDataT<Model>, Model, PoissonNoise1DObjective>
simulate_image(const Model &model, const StencilT<Model> &s, rng_t &rng)
{
    auto sim_im = model.make_image();
    for(ImageCoordT<Model> i=0; i<model.get_size(); i++) sim_im(i) = generate_poisson(rng, model.pixel_model_value(i,s));
    return sim_im;
}

/** @brief Simulate an image at a given theta stencil, by generating Poisson noise
    * Enabled for PoissonNoise1DObjective
    * @param[in] model Model object
    * @param[in] model_im An image representing the expected (mean) at each pixel under the PSF model.
    * @param[in,out] rng A random number generator
    * @returns A simulated image corresponding to model_im under the noise model.
    */
template<class Model, class rng_t>
ReturnIfSubclassT<ModelDataT<Model>, Model, PoissonNoise1DObjective>
simulate_image_from_model(const Model &model, const ImageT<Model> &model_im, rng_t &rng)
{
    auto sim_im = model.make_image();
    for(ImageCoordT<Model> i=0; i<model.get_size(); i++) sim_im(i) = generate_poisson(rng,model_im(i));
    return sim_im;
}

/** @brief Compute the expected information (Fisher information at theta).
    * Note: Expected information is an average quantity and is independent of the data.
    * Enabled for PoissonNoise1DObjective
    * @param model PointEmitterModel
    * @param s Stencil at desired theta
    * @returns The fisher information matrix as an symmetric matrix in upper-triangular format
    */
template<class Model>
ReturnIfSubclassT<MatT, Model, PoissonNoise1DObjective>
expected_information(const Model &model, const StencilT<Model> &s)
{
    auto fisherI = model.make_param_mat(arma::fill::zeros);
    auto pgrad = model.make_param();
    for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {  
        auto model_val = model.pixel_model_value(i,s);
        model.pixel_grad(i,s,pgrad);
        for(IdxT c=0; c<model.get_num_params(); c++) for(IdxT r=0; r<=c; r++) {
            fisherI(r,c) += pgrad(r)*pgrad(c)/model_val; //Fill upper triangle
        }
    }
    return fisherI;
}

template<class Model>    
ReturnIfSubclassT<std::unique_ptr<Estimator<Model>>, Model, PoissonNoise1DObjective>
make_estimator(Model &model, std::string ename)
{
    auto name = ename.c_str();
    if (istarts_with(name,"Heuristic")) {
        return make_unique<HeuristicEstimator<Model>>(model);
    } else if (istarts_with(name,"NewtonDiagonal")) {
        return make_unique<NewtonDiagonalMaximizer<Model>>(model);
    } else if (istarts_with(name,"QuasiNewton")) {
        return make_unique<QuasiNewtonMaximizer<Model>>(model);
    } else if (istarts_with(name,"Newton")) {
        return make_unique<NewtonMaximizer<Model>>(model);
    } else if (istarts_with(name,"TrustRegion")) {
        return make_unique<TrustRegionMaximizer<Model>>(model);
    } else if (istarts_with(name,"SimulatedAnnealing")) {
        return make_unique<SimulatedAnnealingMaximizer<Model>>(model);
    } else {
        std::ostringstream msg;
        msg<<"Unknown estimator name: "<<name;
        throw NotImplementedError(msg.str());
    }
}

/* Core likelihood computations for PoissonNoise1DObjective Models.  No interaction with prior.
    */
namespace likelihood {
    template<class Model>
    ReturnIfSubclassT<double,Model,PoissonNoise1DObjective> 
    llh(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s)
    {
        double llh_val = 0.;
        for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
            if(!std::isfinite(data_im(i))) continue; /* Masked pixels are marked infinite. Skip. */
            llh_val += poisson_log_likelihood(model.pixel_model_value(i,s), data_im(i));
        }
        return llh_val;
    }
    
    template<class Model>
    ReturnIfSubclassT<double,Model,PoissonNoise1DObjective> 
    rllh(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s)
    {
        double rllh_val = 0.;
        for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
            if(!std::isfinite(data_im(i))) continue; /* Masked pixels are marked infinite. Skip. */
            rllh_val += relative_poisson_log_likelihood(model.pixel_model_value(i,s), data_im(i));
        }
        return rllh_val;
    }
    
    template<class Model>
    ReturnIfSubclassT<ParamT<Model>,Model,PoissonNoise1DObjective> 
    grad(const Model &model, const ModelDataT<Model> &im, const StencilT<Model> &s) 
    {
        auto pixel_grad = model.make_param(); 
        auto grad_val = model.make_param(); //Accumulator for overall grad
        grad_val.zeros();
        for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
            if(!std::isfinite(im(i))) continue; /* Masked pixels are marked infinite. Skip. */
            model.pixel_grad(i,s,pixel_grad);
            double model_val = model.pixel_model_value(i,s);
            double dm_ratio_m1 = im(i)/model_val - 1.;
            grad_val += dm_ratio_m1*pixel_grad;
        }
        return grad_val;
    }

    template<class Model>
    ReturnIfSubclassT<void,Model,PoissonNoise1DObjective>
    grad2(const Model &model, const ModelDataT<Model> &im, const StencilT<Model> &s, 
                        ParamT<Model> &grad_val, ParamT<Model> &grad2_val) 
    {
        auto pixel_grad = model.make_param();
        auto pixel_grad2 = model.make_param();
        grad_val.zeros();
        grad2_val.zeros();
        for(ImageCoordT<Model> i=0; i<model.get_size(); i++){
            if(!std::isfinite(im(i))) continue; /* Skip non-finite image values as they are assumed masked */
            /* Compute model value and ratios */
            double model_val = model.pixel_model_value(i,s);
            double dm_ratio = im(i)/model_val;
            double dm_ratio_m1 = dm_ratio-1;
            double dmm_ratio = dm_ratio/model_val;
            model.pixel_grad(i,s,pixel_grad);
            model.pixel_grad2(i,s,pixel_grad2);
            grad_val  += dm_ratio_m1*pixel_grad;
            grad2_val += dm_ratio_m1*pixel_grad2 - dmm_ratio*pixel_grad%pixel_grad;
        }
    }

    template<class Model>
    ReturnIfSubclassT<void,Model,PoissonNoise1DObjective>
    hessian(const Model &model, const ModelDataT<Model> &im, const StencilT<Model> &s, 
                ParamT<Model> &grad_val, MatT &hess_val) 
    {
        /* Returns hessian as an upper triangular matrix */
        grad_val.zeros();
        hess_val.zeros();
        for(typename Model::ImageSizeT i=0;i<model.get_size();i++) { 
            if(!std::isfinite(im(i))) continue; /* Skip non-finite image values as they are assumed masked */
            /* Compute model value and ratios */
            double model_val = model.pixel_model_value(i,s);
            double dm_ratio = im(i)/model_val;
            double dm_ratio_m1 = dm_ratio-1;
            double dmm_ratio = dm_ratio/model_val;
            model.pixel_hess_update(i,s,dm_ratio_m1,dmm_ratio,grad_val,hess_val);
        }
    }
    
    inline namespace debug {
        template<class Model>
        ReturnIfSubclassT<VecT,Model,PoissonNoise1DObjective>
        llh_components(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s)
        {
            VecT llh_vec(model.get_num_pixels(),arma::fill::zeros);
            for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
                if(!std::isfinite(data_im(i))) continue; /* Masked pixels are marked infinite. Skip. */
                llh_vec(i) = poisson_log_likelihood(model.pixel_model_value(i,s), data_im(i));
            }
            return llh_vec;
        }

        template<class Model>
        ReturnIfSubclassT<VecT,Model,PoissonNoise1DObjective>
        rllh_components(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s)
        {
            VecT rllh_vec(model.get_num_pixels(),arma::fill::zeros);
            for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
                if(!std::isfinite(data_im(i))) continue; /* Masked pixels are marked infinite. Skip. */
                rllh_vec(i) = relative_poisson_log_likelihood(model.pixel_model_value(i,s), data_im(i));
            }
            return rllh_vec;
        }
                
        template<class Model>
        ReturnIfSubclassT<MatT,Model,PoissonNoise1DObjective>
        grad_components(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s) 
        {
            auto pixel_grad = model.make_param(); 
            MatT grad_vec(model.get_num_params(),model.get_num_pixels(),arma::fill::zeros); //per-pixel grad contributions to objective
            for(ImageCoordT<Model> i=0; i<model.get_size(); i++) {
                if(!std::isfinite(data_im(i))) continue; /* Masked pixels are marked infinite. Skip. */
                model.pixel_grad(i,s,pixel_grad);
                double model_val = model.pixel_model_value(i,s);
                double dm_ratio_m1 = data_im(i)/model_val - 1.;
                grad_vec.col(i) = dm_ratio_m1*pixel_grad;
            }
            return grad_vec;
        }
        
        template<class Model>
        ReturnIfSubclassT<CubeT,Model,PoissonNoise1DObjective>
        hessian_components(const Model &model, const ModelDataT<Model> &data_im, const StencilT<Model> &s) 
        {
            /* Returns hessian as an upper triangular matrix */
            auto grad_val = model.make_param();
            auto hess_val = model.make_param_mat();
            CubeT hess_vec(model.get_num_params(),model.get_num_params(),model.get_num_pixels(),arma::fill::zeros); //per-pixel grad contributions to objective
            for(typename Model::ImageSizeT i=0;i<model.get_size();i++) { 
                if(!std::isfinite(data_im(i))) continue; /* Skip non-finite image values as they are assumed masked */
                /* Compute model value and ratios */
                double model_val = model.pixel_model_value(i,s);
                double dm_ratio = data_im(i)/model_val;
                double dm_ratio_m1 = dm_ratio-1;
                double dmm_ratio = dm_ratio/model_val;
                grad_val.zeros();
                hess_val.zeros();
                model.pixel_hess_update(i,s,dm_ratio_m1,dmm_ratio,grad_val,hess_val);
                hess_vec.slice(i) = hess_val;
            }
            return hess_vec;
        }

    } /* namespace mappel::methods::likelihood::debug */

} /* namespace mappel::methods::likelihood */
    
} /* namespace mappel::methods */

} /* namespace mappel */

#endif /* _MAPPEL_POISSONNOISE1DOBJECTIVE_H */
