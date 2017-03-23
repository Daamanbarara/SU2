/*!
 * \file variable_direct_2phase.cpp
 * \brief Definition of the solution fields.
 * \author F. Palacios, A. Bueno
 * \version 5.0.0 "Raven"
 *
 * SU2 Lead Developers: Dr. Francisco Palacios (Francisco.D.Palacios@boeing.com).
 *                      Dr. Thomas D. Economon (economon@stanford.edu).
 *
 * SU2 Developers: Prof. Juan J. Alonso's group at Stanford University.
 *                 Prof. Piero Colonna's group at Delft University of Technology.
 *                 Prof. Nicolas R. Gauger's group at Kaiserslautern University of Technology.
 *                 Prof. Alberto Guardone's group at Polytechnic University of Milan.
 *                 Prof. Rafael Palacios' group at Imperial College London.
 *                 Prof. Edwin van der Weide's group at the University of Twente.
 *                 Prof. Vincent Terrapon's group at the University of Liege.
 *
 * Copyright (C) 2012-2017 SU2, the open-source CFD code.
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/variable_structure.hpp"

C2phaseVariable::C2phaseVariable(void) : CVariable() {

	Source = 0; Enthalpy_Liquid = 0;
	Radius = 0; Liquid_Fraction = 0;

	Primitive_Liquid = new su2double [9];

}

C2phaseVariable::C2phaseVariable(unsigned short val_nDim, unsigned short val_nvar, CConfig *config)
: CVariable(val_nDim, val_nvar, config) {
  
  unsigned short iVar;

  /*--- Allocate space for the limiter ---*/
  
  Limiter = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++)
    Limiter[iVar] = 0.0;
  
  Solution_Max = new su2double [nVar];
  Solution_Min = new su2double [nVar];
  for (iVar = 0; iVar < nVar; iVar++) {
    Solution_Max[iVar] = 0.0;
    Solution_Min[iVar] = 0.0;
  }
  
}
/*
C2phaseVariable::SetSource(su2double S) {Source = S; };

C2phaseVariable::SetLiqEnthalpy(su2double h) {Enthalpy_Liquid = h; };

C2phaseVariable::SetRadius(su2double R)     {Radius = R;};

C2phaseVariable::SetLiquidFrac(su2double Y) {Liquid_Fraction = Y; };
*/

C2phaseVariable::~C2phaseVariable(void) {
	if (Primitive_Liquid != NULL) delete [] Primitive_Liquid;
}



C2phase_HillVariable::C2phase_HillVariable(void) : C2phaseVariable() {

	//V_l = new su2double [9];
}

C2phase_HillVariable::C2phase_HillVariable(su2double val_R, su2double val_N, su2double rho_m, CConfig *config): C2phaseVariable() {

  bool dual_time = ((config->GetUnsteady_Simulation() == DT_STEPPING_1ST) ||
                    (config->GetUnsteady_Simulation() == DT_STEPPING_2ND));
  

  /*--- Initialization of variables ---*/
  
  Solution[0] = rho_m * val_N;                Solution_Old[0] = rho_m * val_N;
  Solution[1] = rho_m * val_N*val_R;          Solution_Old[1] = rho_m * val_N*val_R;
  Solution[2] = rho_m * val_N*pow(val_R, 2);  Solution_Old[2] = rho_m * val_N*pow(val_R, 2);
  Solution[3] = rho_m * val_N*pow(val_R, 3);  Solution_Old[3] = rho_m * val_N*pow(val_R, 3);


  /*--- Allocate and initialize solution for the dual time strategy ---*/
  
  if (dual_time) {
	Solution_time_n[0] = rho_m * val_N;                Solution_time_n1[0] = rho_m * val_N;
	Solution_time_n[1] = rho_m * val_N*val_R;          Solution_time_n1[1] = rho_m * val_N*val_R;
	Solution_time_n[2] = rho_m * val_N*pow(val_R, 2);  Solution_time_n1[2] = rho_m * val_N*pow(val_R, 2);
	Solution_time_n[3] = rho_m * val_N*pow(val_R, 3);  Solution_time_n1[3] = rho_m * val_N*pow(val_R, 3);
  }
    
}

C2phase_HillVariable::~C2phase_HillVariable(void) {

	//if (V_l != NULL) delete [] V_l;
  
}

void C2phase_HillVariable::SetDropletProp(su2double rho_l, su2double rho_v, su2double G) {

	R = Solution[1]/Solution[0];

    y = Solution[3]*(rho_l - rho_v);
    y = y + 0.75 * rho_v / 3.14;
    y = y*rho_l / y;

    rho_m = y/ rho_l + (1.0 - y)/ rho_v;
    rho_m = 1.0/ rho_m;

    N = Solution[0]/rho_m;

    Source = rho_m * 3 * y / R * G;

}

void C2phase_HillVariable::SetLiquidPrim(su2double *Primitive, su2double *Two_Phase_Var,CConfig *config) {

	su2double rho_l, rho_m, T_l, h_l, Psat, Tsat, sigma, Rc, R;

	P   = Primitive[nDim+1] / config->GetPressure_Ref();
	T   = Primitive[0]      / config->GetTemperature_Ref();
	rho = Primitive[nDim+2] / config->GetDensity_Ref();
	h   = Primitive[nDim+3] / config->GetEnergy_Ref();

	FluidModel->SetLiquidProp(P, T, rho, h, Two_Phase_Var);

	T_l = FluidModel->GetLiquidTemperature()/config->GetTemperature_Ref();

	rho_l = FluidModel->GetLiquidDensity()/config->GetDensity_Ref();

	h_l = FluidModel->GetLiquidEnthalpy()/config->GetEnergy_Ref();

	Psat = FluidModel->GetPsat()/config->GetPressure_Ref();

	Tsat = FluidModel->GetTsat()/config->GetTemperature_Ref();

	sigma = FluidModel->GetSurfaceTension()/config->GetSurfTension_Ref();

	Rc = FluidModel->GetCriticalRadius()/config->GetLength_Ref();

	R = FluidModel->GetRadius()/config->GetLength_Ref();

	rho_m = FluidModel->GetMixtureDensity()/config->GetDensity_Ref();


	Primitive_Liquid[0] = T_l;
	Primitive_Liquid[1] = rho_l;
	Primitive_Liquid[2] = h_l;
	Primitive_Liquid[3] = Psat;
	Primitive_Liquid[4] = Tsat;
	Primitive_Liquid[5] = sigma;
	Primitive_Liquid[6] = Rc;
	Primitive_Liquid[7] = R;
	Primitive_Liquid[8] = rho_m;


}



