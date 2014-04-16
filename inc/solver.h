#ifndef SOLVER_H_
#define SOLVER_H_

#include "main.h"
#include <memory>

class Parallel;
class Grid;
class Impulse;
class Gas;
class GridManager;
class Config;
class Solver;

typedef std::vector<std::shared_ptr<Gas>> GasVector;

class Solver {
public:
  Solver();

  void Init(GridManager* pGridManager);
  void Run();
  
  Parallel* GetParallel() const { return m_pParallel.get(); }
	Impulse* GetImpulse() const { return m_pImpulse.get(); }
  GasVector& GetGas() { return m_vGas; }

private:
  void InitCellType(sep::Axis axis);
  void MakeStep(sep::Axis axis);
  void MakeIntegral(unsigned int gi0, unsigned int gi1, double timestep);
  void PreRun();
  
  std::shared_ptr<Parallel> m_pParallel;
	std::shared_ptr<Impulse> m_pImpulse;
	GasVector m_vGas;
  
  GridManager* m_pGridManager;
  Grid* m_pGrid;
  Config* m_pConfig;
};

#endif /* SOLVER_H_ */
