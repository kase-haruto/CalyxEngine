/*-----------------------------------------------------------------------------------------
 * BaseBullet
 * - 弾の基底クラス
 * - 全ての弾オブジェクトに共通する移動、寿命管理、衝突判定の基本挙動を定義
 *---------------------------------------------------------------------------------------*/
class BaseBullet :
	public Actor{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief デフォルトコンストラクタ
	 */
	BaseBullet() = default;

	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param name オブジェクト名
	 */
	BaseBullet(const std::string& modelName,const std::string& name);

	/**
	 * \brief デストラクタ
	 */
	virtual ~BaseBullet()override = default;

	/**
	 * \brief 発射時処理
	 */
	virtual void OnShot();

	/**
	 * \brief 射撃初期化
	 * \param initPos 初期座標
	 * \param velocity 初速
	 */
	virtual void ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity);

	/**
	 * \brief 初期化
	 */
	void Initialize() override {}

	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt)override;

	/**
	 * \brief 派生クラス用GUI表示
	 */
	void DerivativeGui()override;

	//--------- collider -------------------------------------------------
	/**
	 * \brief 衝突開始時処理
	 * \param other 衝突相手のコライダー
	 */
	void OnCollisionEnter(Collider* other)override;

	/**
	 * \brief 衝突継続時処理
	 */
	void OnCollisionStay([[maybe_unused]] Collider* other)override {}

	/**
	 * \brief 衝突終了時処理
	 */
	void OnCollisionExit([[maybe_unused]] Collider* other)override {}

	//--------- accessor -------------------------------------------------
	/**
	 * \brief コライダーを取得
	 * \return コライダー
	 */
	Collider* GetCollider() { return BaseGameObject::GetCollider(); }

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	float lifeTime_ = 3.0f;      //< 弾の寿命（秒）
	float currentTime_ = 0.0f;   //< 経過時間
};

