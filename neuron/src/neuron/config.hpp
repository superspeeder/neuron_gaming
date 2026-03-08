//
// Created by andy on 3/8/26.
//

#pragma once

#ifdef WIN32
#  ifdef NEURON_SHARED
#    ifdef NEURON_EXPORTS
#      define NEURON_API __declspec(dllexport)
#    else
#      define NEURON_API __declspec(dllimport)
#    endif
#  else
#    define NEURON_API
#  endif
#else
#  define NEURON_API
#endif

