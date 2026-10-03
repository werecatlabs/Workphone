


class UVLookup
{
public:
	UVLookup();
	~UVLookup();

	void load(const String& filePath);
	void save(const String& filePath);

	Vector2F getUV(const Vector2F& position);

	void add(const Vector2F& position, const Vector2F& uv);
	bool remove(const Vector2F& position);

protected:
	Grid2 m_grid;

	typedef Array<Vector2F> UVLookupMap;
	UVLookupMap m_uvLookupMap;
};