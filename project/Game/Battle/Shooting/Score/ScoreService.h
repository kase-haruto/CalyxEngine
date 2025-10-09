#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <queue>

struct GainScore;

class ScoreService {
  public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	ScoreService();
	~ScoreService();

	void Initialize();
	void Shutdown(); // 購読解除
	void Update();	 // キューをドレインして合算

	void AddRaw(int v);
	int	 GetTotal() const { return total_; }

  private:
	//===================================================================*/
	//					private func
	//===================================================================*/
	void OnGainScore(const GainScore& ev);

  private:
	//===================================================================*/
	//					private variable
	//===================================================================*/
	int total_ = 0;
	struct Pending {
		int amount;
	};
	std::queue<Pending> q_;
};