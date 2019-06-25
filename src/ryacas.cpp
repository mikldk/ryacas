#include <Rcpp.h>

// [[Rcpp::plugins(cpp11)]]

#include <yacas/yacas.h>

#include <sstream>

namespace {
    static std::stringstream _side_effects;
    static CYacas* _yacas = nullptr;

    static
    void yacas_initialize(std::string alternative_path)
    {
        _yacas = new CYacas(_side_effects);        
        
        Rcpp::Environment base_env = Rcpp::Environment::base_env();
        Rcpp::Function system_file = base_env["system.file"];
        
        std::string scripts_path = Rcpp::as<std::string>(system_file(Rcpp::Named("package", "Ryacas"), "yacas"));
        
        // During development, the path can be overwritten
        if (!alternative_path.empty()) {
          scripts_path = alternative_path;
          Rcpp::Rcout << " - Searching for yacas at \"" << scripts_path << "\"" << std::endl;
        }

        if (!scripts_path.empty()) {
            if (scripts_path.back() != '/')
                scripts_path.push_back('/');
            _yacas->Evaluate(std::string("DefaultDirectory(\"") +  scripts_path + "\");");
        }

        if (!_yacas->IsError())
            _yacas->Evaluate("Load(\"yacasinit.ys\");");

        if (!_yacas->IsError()) {
          // DEPRECATION: delete:
          _yacas->Evaluate("PrettyPrinter'Set(\"OMForm\");");
        }

        if (_yacas->IsError()) {
            const std::string msg = "Failed to initialize yacas: " + _yacas->Error();
            
            _yacas = nullptr;
            Rcpp::stop(msg);
        }
    }
}

// [[Rcpp::export]]
void yacas_init_force(std::string path)
{
  Rcpp::Rcout << "Trying to initialise internal yacas: " << std::endl;
  yacas_initialize(path);
  Rcpp::Rcout << "Done." << std::endl;
}

//' Evaluate yacas expression
//' 
//' This is a low-level function for evaluating yacas expression represented as
//' string.
//' 
//' @param expr Yacas expression
//' @return Result of evaluating \code{expr} by yacas and
//' side-effects of the evaluation
//' 
//' @examples
//' yac_core("D(x)Sin(x^2)")
//' 
//' @concept yac_communication
//' @keywords internal
//' 
// [[Rcpp::export]]
std::vector<std::string> yac_core(std::string expr)
{
  if (!_yacas) {
    yacas_initialize(std::string());
  }
  
  _side_effects.clear();
  _side_effects.str("");
  
  // NOTE: Until DEPRECATED is removed, totally,
  //       set printer back to standard and restore
  // DEPRECATION: delete:
  _yacas->Evaluate("PrettyPrinter'Set();");
  _yacas->Evaluate(expr);
  
  if (_yacas->IsError()) {
    std::string err = _yacas->Error();
    std::string msg = "Yacas returned this error: " + err;
    
    // DEPRECATION: delete:
    _yacas->Evaluate("PrettyPrinter'Set(\"OMForm\");");
    
    Rcpp::stop(msg);
  }
  
  const std::vector<std::string> results = {
    _side_effects.str(),
    _yacas->Result()
  };
  
  // DEPRECATION: delete:
  _yacas->Evaluate("PrettyPrinter'Set(\"OMForm\");");
  
  return results;
}
