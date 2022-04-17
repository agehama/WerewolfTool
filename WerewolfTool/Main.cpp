#include <Siv3D.hpp> // OpenSiv3D v0.6.3

inline std::pair<Vec2, Vec2> FixedPosVel(const Vec2& pos, const Vec2& vel, const RectF& rect)
{
	if (rect.intersects(pos))
	{
		return { pos, vel };
	}

	const auto isOuter = [&](const Line& line)
	{
		return line.vector().cross(pos - line.begin) < 0.0;
	};

	const Line topLine(rect.tl(), rect.tr());
	const Line rightLine(rect.tr(), rect.br());
	const Line bottomLine(rect.br(), rect.bl());
	const Line leftLine(rect.bl(), rect.tl());

	const Line realTopLine(rect.tl(), rect.tr() + Vec2(1, 0));
	const Line realRightLine(rect.tr() + Vec2(1, 0), rect.br() + Vec2(1, 1));
	const Line realBottomLine(rect.br() + Vec2(1, 1), rect.bl() + Vec2(0, 1));
	const Line realLeftLine(rect.bl() + Vec2(0, 1), rect.tl());

	/*
	0 | 1 | 2
	---------
	6 |   | 7
	---------
	5 | 4 | 3
	*/

	if (isOuter(topLine))
	{
		//case 0
		if (isOuter(leftLine))return { rect.tl(), Vec2::Zero() };
		//case 2
		if (isOuter(rightLine))return { rect.tr(), Vec2::Zero() };
		//case 1
		return { realTopLine.closest(pos), Vec2(vel.x,0) };
	}

	if (isOuter(bottomLine))
	{
		//case 3
		if (isOuter(rightLine))return { rect.br(), Vec2::Zero() };
		//case 5
		if (isOuter(leftLine))return { rect.bl(), Vec2::Zero() };
		//case 4
		return { realBottomLine.closest(pos), Vec2(vel.x,0) };
	}

	//case 6
	if (isOuter(leftLine))return { realLeftLine.closest(pos), Vec2(0,vel.y) };
	//case 7
	return { realRightLine.closest(pos), Vec2(0,vel.y) };
};

inline Vec2 FixedRectPos(const RectF& rect, const RectF& scope)
{
	if (scope.contains(rect))
	{
		return rect.pos;
	}

	const Vec2 fixVecBL = FixedPosVel(rect.bl(), Vec2::Zero(), scope).first - rect.bl();
	const auto blFixedRect = rect.movedBy(fixVecBL);
	if (scope.contains(blFixedRect))
	{
		return blFixedRect.pos;
	}

	const Vec2 fixVecTR = FixedPosVel(rect.tr(), Vec2::Zero(), scope).first - rect.tr();
	const auto trFixedRect = rect.movedBy(fixVecTR);
	if (scope.contains(trFixedRect))
	{
		return trFixedRect.pos;
	}

	const Vec2 fixVecBR = FixedPosVel(rect.br(), Vec2::Zero(), scope).first - rect.br();
	return rect.movedBy(fixVecBR).pos;
}

inline Optional<Line> CutoffLine(const Line& original, double lengthBegin, double lengthEnd)
{
	const Vec2 v = original.vector().normalize();
	const double l = lengthBegin + lengthEnd;
	if (original.lengthSq() < l * l)
	{
		return none;
	}
	return Line(original.begin + v * lengthBegin, original.end - v * lengthEnd);
};

inline void DrawBR(const Vec2& bottomRight, const DrawableText& str, const Color& color = Palette::White)
{
	str.draw(bottomRight - str.region().size, color);
};

struct Character
{
	Character() = default;
	Character(const String& name, const Texture& texture, const Vec2& pos, bool isActive = false)
		: name(name)
		, texture(texture)
		, position(position)
		, isActive(isActive)
	{}

	virtual void draw(const Font& font, const Font& fontDeathCause, const Color& overlayColor = Alpha(0))const
	{
		texture.drawAt(position).draw(overlayColor);
		font(name).draw(position - texture.size() * 0.5 + Vec2(1, 2), Palette::Black);
		font(name).draw(position - texture.size() * 0.5, isActive ? Palette::Red : Palette::White);
	}

	RectF rect()const
	{
		return RectF(texture.size()).setCenter(position);
	}

	String name;
	Texture texture;
	Vec2 position;
	bool isActive;
};

struct CharacterNode : public Character
{
	CharacterNode() = default;

	CharacterNode(const String& name, const Texture& texture, const Vec2& pos, bool isActive = false)
		: Character(name, texture, pos, isActive)
	{
		initRadius();
	}

	CharacterNode(const Character& character)
		: Character(character)
	{
		initRadius();
	}

	void initRadius()
	{
		radius = 0.5 * sqrt(texture.width() * texture.width() + texture.height() * texture.height());
	}

	void draw(const Font& font, const Font& fontDeathCause, const Color& overlayColor = Alpha(0))const override
	{
		if (state == Alive)
		{
			texture.drawAt(position).draw(overlayColor);
		}
		else
		{
			texture.drawAt(position).draw(Color(0, 0, 0, 180)).draw(overlayColor);

			switch (state)
			{
			case CharacterNode::Hanged:
				DrawBR(rect().br(), fontDeathCause(U"吊"), Palette::Red);
				break;
			case CharacterNode::Bitten:
				DrawBR(rect().br(), fontDeathCause(U"噛"), Palette::Red);
				break;
			case CharacterNode::Suddenly:
				DrawBR(rect().br(), fontDeathCause(U"突"), Palette::Red);
				break;
			default:
				break;
			}
		}
		font(name).draw(position - texture.size() * 0.5 + Vec2(1, 2), Palette::Black);
		font(name).draw(position - texture.size() * 0.5, isActive ? Palette::Red : Palette::White);
	}

	RectF rect()const
	{
		return RectF(texture.size()).setCenter(position);
	}

	RectF getFieldScope()const
	{
		return Scene::Rect().stretched(-radius, -radius);
	}

	Color getColor()const
	{
		switch (co)
		{
		case CharacterNode::None:
			return HSV(0, 0, 0.5);
		case CharacterNode::Fortuneteller:
			return HSV(200, 1, 1);
		case CharacterNode::Spiritualist:
			return HSV(300, 1, 1);
		case CharacterNode::Hunter:
			return HSV(90, 1, 1);
		case CharacterNode::Madman:
			return HSV(30, 1, 1);
		default:
			return HSV(0, 0, 0);
		}
	}

	enum Roal { None, Fortuneteller, Spiritualist, Hunter, Madman };
	enum State { Alive, Hanged, Bitten, Suddenly };

	Vec2 velocity;
	double radius;
	Roal co = None;
	bool isAutoLayout = true;
	State state = Alive;
};

//256,384
struct MenuGUI
{
	MenuGUI() = default;
	MenuGUI(int nodeIndex, const Vec2& pos)
		: nodeIndex(nodeIndex)
		, guiPos(FixedRectPos(RectF(pos, 256, 384), Scene::Rect()))
	{}

	int nodeIndex;
	Vec2 guiPos;

	RectF guiRect()const
	{
		return RectF(guiPos, 256, 384);
	}

	RectF buttonFortuneteller()const
	{
		return RectF(3, 3, 122, 122).moveBy(guiPos);
	}
	RectF buttonSpiritualist()const
	{
		return RectF(131, 3, 122, 122).moveBy(guiPos);
	}
	RectF buttonHunter()const
	{
		return RectF(3, 131, 122, 122).moveBy(guiPos);
	}
	RectF buttonMadman()const
	{
		return RectF(131, 131, 122, 122).moveBy(guiPos);
	}
	RectF buttonFixPos()const
	{
		return RectF(3, 259, 122, 122).moveBy(guiPos);
	}

	RectF buttonHang()const
	{
		return RectF(130, 258, 60, 60).moveBy(guiPos);
	}
	RectF buttonBite()const
	{
		return RectF(194, 258, 60, 60).moveBy(guiPos);
	}
	RectF buttonSudden()const
	{
		return RectF(130, 322, 60, 60).moveBy(guiPos);
	}

	std::vector<RectF> buttonsRects()const
	{
		return {
			buttonFortuneteller(),
			buttonSpiritualist(),
			buttonHunter(),
			buttonMadman(),
			buttonFixPos(),
			buttonHang(),
			buttonBite(),
			buttonSudden()
		};
	}

	void update(std::vector<CharacterNode>& nodes)
	{
		CharacterNode& node = nodes[nodeIndex];

		const auto updateCO = [&](CharacterNode::Roal roal)
		{
			//node.co = node.co == roal ? CharacterNode::None : roal;
			if (node.co == roal)
			{
				node.co = CharacterNode::None;
				//node.isAutoLayout = false;
			}
			else
			{
				node.co = roal;
				//node.isAutoLayout = true;
			}
		};

		const auto updateState = [&](CharacterNode::State state)
		{
			node.state = node.state == state ? CharacterNode::Alive : state;
		};

		if (buttonFortuneteller().leftClicked())
		{
			updateCO(CharacterNode::Fortuneteller);
		}
		else if (buttonSpiritualist().leftClicked())
		{
			updateCO(CharacterNode::Spiritualist);
		}
		else if (buttonHunter().leftClicked())
		{
			updateCO(CharacterNode::Hunter);
		}
		else if (buttonMadman().leftClicked())
		{
			updateCO(CharacterNode::Madman);
		}
		else if (buttonFixPos().leftClicked())
		{
			node.isAutoLayout = !node.isAutoLayout;
		}
		else if (buttonHang().leftClicked())
		{
			updateState(CharacterNode::State::Hanged);
		}
		else if (buttonBite().leftClicked())
		{
			updateState(CharacterNode::State::Bitten);
		}
		else if (buttonSudden().leftClicked())
		{
			updateState(CharacterNode::State::Suddenly);
		}
	}

	void draw(const Texture& texture, const std::vector<CharacterNode>& nodes)const
	{
		texture.draw(guiPos);

		const CharacterNode& node = nodes[nodeIndex];

		Graphics3D::Internal::SetBlendState(BlendState::Additive);

		const auto currentRoalColor = node.getColor().setA(64);
		switch (node.co)
		{
		case CharacterNode::Fortuneteller:
			buttonFortuneteller().draw(currentRoalColor);
			break;
		case CharacterNode::Spiritualist:
			buttonSpiritualist().draw(currentRoalColor);
			break;
		case CharacterNode::Hunter:
			buttonHunter().draw(currentRoalColor);
			break;
		case CharacterNode::Madman:
			buttonMadman().draw(currentRoalColor);
			break;
		default:
			break;
		}

		if (node.isAutoLayout)
		{
			buttonFixPos().draw(HSV(30, 1, 1).toColor(64));
		}

		switch (node.state)
		{
		case CharacterNode::State::Hanged:
			buttonHang().draw(HSV(0, 1, 1).toColor(64));
			break;
		case CharacterNode::State::Bitten:
			buttonBite().draw(HSV(0, 1, 1).toColor(64));
			break;
		case CharacterNode::State::Suddenly:
			buttonSudden().draw(HSV(0, 1, 1).toColor(64));
			break;
		default:
			break;
		}

		for (const auto& buttonRect : buttonsRects())
		{
			if (buttonRect.mouseOver())
			{
				buttonRect.draw(Alpha(32));
			}
		}

		Graphics3D::Internal::SetBlendState(BlendState::Default2D);

		node.texture.scaled(30.0 / node.texture.width(), 30.0 / node.texture.height()).draw(guiPos + Vec2(224, 352));
	}
};

//参考資料: http://asus.myds.me:6543/paper/nw/Efficient,%20High-QualityForce-Directed%20GraphDrawing.pdf
class Graph
{
public:
	Graph() = default;
	Graph(const std::vector<CharacterNode>& nodes)
		: adjacents(nodes.size(), std::vector<char>(nodes.size(), 0))
		, menuTexture(U"Resource/gui.png")
	{}

	void initialize(const std::vector<CharacterNode>& nodes)
	{
		adjacents = std::vector<std::vector<char>>(nodes.size(), std::vector<char>(nodes.size(), 0));
		menuTexture = Texture(U"Resource/gui.png");
		linkBeginIndex = none;
		moveIndex = none;
		characterGUI = none;

		continueSimulation = true;
	}

	void setLink(int indexFrom, int indexTo, char isEnabled)
	{
		adjacents[indexFrom][indexTo] = isEnabled;
	}

	void update(std::vector<CharacterNode>& nodes)
	{
		for (auto i : step(nodes.size()))
		{
			//NaN対策：二つのノードが完全に重なったとき反発力がNaNになる
			//二つのノードが完全に重なることは基本無いがFixedPosVelで位置が四隅に補正された場合はあり得る
			if (nodes[i].position != nodes[i].position || nodes[i].velocity != nodes[i].velocity)
			{
				auto posVel = FixedPosVel(RandomVec2(Scene::Rect()), Vec2::Zero(), Scene::Rect());
				nodes[i].position = posVel.first;
				nodes[i].velocity = Vec2::Zero();
			}
		}

		inputsUpdate(nodes);
		physicsUpdate(nodes);
	}

	void draw(const std::vector<CharacterNode>& nodes, const Font& characterNameFont, const Font& characterDeathCauseFont)const
	{
		for (auto i : step(nodes.size()))
		{
			const Circle circle(nodes[i].position, nodes[i].radius);
			circle.drawFrame(5.0, 0.0, nodes[i].getColor());
			nodes[i].draw(characterNameFont, characterDeathCauseFont);
		}

		//リンクの描画;
		for (int me = 0; me < nodes.size(); ++me)
		{
			for (int other = 0; other < nodes.size(); ++other)
			{
				if (me == other)
				{
					continue;
				}

				if (adjacents[me][other] != 0)
				{
					const auto color = adjacents[me][other] == 1 ? Palette::White : Palette::Black;
					if (auto arrow = CutoffLine(Line(nodes[me].position, nodes[other].position), nodes[me].radius, nodes[other].radius))
					{
						arrow.value().drawArrow(3.0, { 20,20 }, color);
					}

				}
			}
		}

		if (linkBeginIndex && !Circle(nodes[linkBeginIndex.value()].position, nodes[linkBeginIndex.value()].radius).mouseOver())
		{
			if (auto arrow = CutoffLine(Line(nodes[linkBeginIndex.value()].position, Cursor::PosF()), nodes[linkBeginIndex.value()].radius, 0))
			{
				arrow.value().drawArrow(3.0, { 15,15 }, Alpha(128));
			}
		}

		if (characterGUI)
		{
			characterGUI.value().draw(menuTexture, nodes);
		}

		if (linkEraseBegin)
		{
			Line(linkEraseBegin.value(), Cursor::PosF()).draw(3.0, Color(255, 0, 0, 128));
		}
	}

private:
	void inputsUpdate(std::vector<CharacterNode>& nodes)
	{
		if (!linkBeginIndex && !moveIndex && !characterGUI && !linkEraseBegin)
		{
			for (int i : step(nodes.size()))
			{
				const int index = nodes.size() - 1 - i;
				const Circle circle(nodes[index].position, nodes[index].radius);
				if (circle.rightClicked())
				{
					moveIndex = index;
					break;
				}
				if (circle.leftClicked())
				{
					linkBeginIndex = index;
					break;
				}
			}

			if (MouseL.down() && !moveIndex && !linkBeginIndex)
			{
				linkEraseBegin = Cursor::PosF();
			}
		}
		else if (moveIndex)
		{
			if (MouseR.pressed())
			{
				nodes[moveIndex.value()].position = FixedPosVel(Cursor::PosF(), Vec2::Zero(), nodes[moveIndex.value()].getFieldScope()).first;
			}
			if (MouseR.up())
			{
				characterGUI = none;
				moveIndex = none;
				linkBeginIndex = none;
				linkEraseBegin = none;
			}
		}
		else if (linkBeginIndex)
		{
			if (MouseL.up())
			{
				Optional<int> linkEndIndex;
				for (auto i : step(nodes.size()))
				{
					if (Circle(nodes[i].position, nodes[i].radius).mouseOver())
					{
						if (i == linkBeginIndex.value())
						{
							characterGUI = MenuGUI(i, Cursor::PosF());
							break;
						}
						else
						{
							linkEndIndex = i;
							break;
						}
					}
				}

				if (linkEndIndex)
				{
					const int color = KeyAlt.pressed() ? 2 : 1;
					setLink(linkBeginIndex.value(), linkEndIndex.value(), color);
				}

				moveIndex = none;
				linkBeginIndex = none;
				linkEraseBegin = none;
			}
		}
		else if (characterGUI)
		{
			characterGUI.value().update(nodes);
			if (MouseL.down())
			{
				if (!characterGUI.value().guiRect().mouseOver())
				{
					characterGUI = none;
					moveIndex = none;
					linkBeginIndex = none;
					linkEraseBegin = none;
				}
			}
		}
		else if (linkEraseBegin)
		{
			if (!MouseL.pressed())
			{
				const Line eracerLine(linkEraseBegin.value(), Cursor::PosF());

				for (int me = 0; me < nodes.size(); ++me)
				{
					for (int other = 0; other < nodes.size(); ++other)
					{
						if (me == other)
						{
							continue;
						}

						if (adjacents[me][other] != 0)
						{
							if (auto arrow = CutoffLine(Line(nodes[me].position, nodes[other].position), nodes[me].radius, nodes[other].radius))
							{
								if (arrow.value().intersects(eracerLine))
								{
									setLink(me, other, 0);
									setLink(other, me, 0);
								}
							}
							//if (CutoffLine(Line(nodes[me].position, nodes[other].position), nodes[me].radius, nodes[other].radius).intersects(eracerLine))
							//{
							//	setLink(me, other, 0);
							//	setLink(other, me, 0);
							//}
						}
					}
				}

				characterGUI = none;
				moveIndex = none;
				linkBeginIndex = none;
				linkEraseBegin = none;
			}
		}
	}

	void physicsUpdate(std::vector<CharacterNode>& nodes)
	{
		if (KeySpace.down())
		{
			continueSimulation = !continueSimulation;
		}

		if (!continueSimulation)
		{
			return;
		}

		const double naturalDistance = 300.0;
		const double relativeStrength = 0.2;
		//const double relativeStrength = 0.4;
		//const double relativeStrength = 0.1;

		//const double relativeStrength = 0.6;
		//const double relativeStrength = 1.0;
		//const double naturalDistance = 500.0;
		//const double relativeStrength = 0.1;

		const double K = naturalDistance;
		const double K2 = K * K;
		const double C = relativeStrength;

		//const double dt = 0.0167;
		const double dt = 0.005;

		for (int i = 0; i < 10; ++i)
		{
			std::vector<Vec2> forces(nodes.size(), Vec2::Zero());
			for (int me = 0; me < nodes.size(); ++me)
			{
				if (!nodes[me].isAutoLayout)
				{
					continue;
				}

				const Vec2 pos_me = nodes[me].position;
				//forces[me] += pos_me.distanceFrom(Window::Center()) * (Window::Center() - pos_me) / K;
				//散らばりすぎないように求心力も少し加える
				forces[me] += 0.5 * pos_me.distanceFrom(Scene::Center()) * (Scene::Center() - pos_me) / K;

				for (int other = 0; other < nodes.size(); ++other)
				{
					if (me == other)
					{
						continue;
					}

					const Vec2 pos_other = nodes[other].position;
					const Vec2 relative = pos_other - pos_me;
					const double distance2 = relative.dot(relative);
					const double distance = sqrt(distance2);

					//全ての頂点間の斥力
					forces[me] += -C * K2 * relative / distance2;
					/*if (1.e-6 < distance2)
					{
						forces[me] += -C * K2 * relative / distance2;
					}
					else
					{
						forces[me] += RandomVec2()*100;
					}*/

					//リンク間の引力
					//運動方程式を解く時はリンクの向きは考慮しない
					if (adjacents[me][other] != 0 || adjacents[other][me] != 0)
					{
						forces[me] += 0.175 * distance * relative / K;
						//forces[me] += (distance - K) * relative;
					}
				}
			}

			for (int me = 0; me < nodes.size(); ++me)
			{
				if (!nodes[me].isAutoLayout)
				{
					nodes[me].velocity = Vec2::Zero();
					continue;
				}

				//const double resistance = 0.999;
				const double resistance = 0.995;

				auto posVel = FixedPosVel(nodes[me].position + nodes[me].velocity * dt, nodes[me].velocity + forces[me] * dt, nodes[me].getFieldScope());
				nodes[me].position = posVel.first;
				nodes[me].velocity = posVel.second * resistance;
			}
		}
	}

	//0: リンクなし, 1:白出し, 2:黒出し
	std::vector<std::vector<char>> adjacents;

	Optional<Vec2> linkEraseBegin;
	Optional<int> linkBeginIndex;
	Optional<int> moveIndex;
	Optional<MenuGUI> characterGUI;

	Texture menuTexture;
	bool continueSimulation = true;
};

class Game
{
public:
	Game(const std::vector<FilePath>& characterImagePaths, const std::vector<FilePath>& characterImagePaths2)
		: characterNameFont(14, Typeface::Heavy)
		, systemFont(32)
		, characterDeathCauseFont(32)
	{
		{
			Image image(U"Resource/gui2.png");
			image.scale(LoadResolution().x, LoadResolution().y);
			characterHideTexture = Texture(image);
		}

		for (const auto& path : characterImagePaths)
		{
			Image image(path);
			image.scale(LoadResolution().x, LoadResolution().y);
			characterTemplates.emplace_back(FileSystem::BaseName(path), Texture(image), RandomVec2(Scene::Rect()));
		}

		for (const auto& path : characterImagePaths2)
		{
			Image image(path);
			image.scale(LoadResolution().x, LoadResolution().y);
			characterTemplates2.emplace_back(FileSystem::BaseName(path), Texture(image), RandomVec2(Scene::Rect()));
		}

		std::sort(characterTemplates.begin(), characterTemplates.end(), [](const Character& a, const Character& b) {return a.name < b.name; });
		std::sort(characterTemplates2.begin(), characterTemplates2.end(), [](const Character& a, const Character& b) {return a.name < b.name; });

		restart();
	}

	void restart()
	{
		state = Initial;
		characters.clear();
		characterHideButton = none;
		showCharacter2 = false;
	}

	void update()
	{
		if (state == Initial)
		{
			const int horizontalNum = Scene::Width() / LoadResolution().x;

			{
				size_t i = 0;
				const Vec2 w = LoadResolution();

				for (; i < characterTemplates.size(); ++i)
				{
					const int x = i % horizontalNum;
					const int y = i / horizontalNum;
					characterTemplates[i].position = w * Vec2(x, y) + w * 0.5;
				}

				if (!characterTemplates2.empty())
				{
					characterHideButton = RectF(w * Vec2(i % horizontalNum, i / horizontalNum), w);
					for (++i; i < 1 + characterTemplates.size() + characterTemplates2.size(); ++i)
					{
						const int x = i % horizontalNum;
						const int y = i / horizontalNum;
						characterTemplates2[i - 1 - characterTemplates.size()].position = w * Vec2(x, y) + w * 0.5;
					}
				}
			}

			for (auto& character : characterTemplates)
			{
				if (character.rect().leftClicked())
				{
					character.isActive = !character.isActive;
				}
			}

			if (characterHideButton)
			{
				if (characterHideButton.value().leftClicked())
				{
					showCharacter2 = !showCharacter2;
				}
			}

			if (showCharacter2)
			{
				for (auto& character : characterTemplates2)
				{
					if (character.rect().leftClicked())
					{
						character.isActive = !character.isActive;
					}
				}
			}

			if (KeyEnter.down())
			{
				//state = Select;

				characters.clear();
				for (const auto& characterTemplate : characterTemplates)
				{
					if (characterTemplate.isActive)
					{
						characters.push_back(characterTemplate);
					}
				}
				for (const auto& characterTemplate : characterTemplates2)
				{
					if (characterTemplate.isActive)
					{
						characters.push_back(characterTemplate);
					}
				}
				for (int j = 0; j < characters.size(); ++j)
				{
					characters[j].isActive = false;
				}
				graph.initialize(characters);

				state = Update;
			}
		}
		else if (state == Select)
		{
			int realIndex = 0;

			const auto switchToUpdate = [&]()
			{
				characters.clear();
				for (const auto& characterTemplate : characterTemplates)
				{
					if (characterTemplate.isActive)
					{
						characters.push_back(characterTemplate);
					}
				}
				for (const auto& characterTemplate : characterTemplates2)
				{
					if (characterTemplate.isActive)
					{
						characters.push_back(characterTemplate);
					}
				}
				for (int j = 0; j < characters.size(); ++j)
				{
					characters[j].isActive = (j == realIndex ? true : false);
				}
				myselfIndex = realIndex;
				state = Update;
				graph.initialize(characters);
			};

			for (int i = 0; i < characterTemplates.size(); ++i)
			{
				if (characterTemplates[i].isActive)
				{
					if (characterTemplates[i].rect().leftClicked())
					{
						switchToUpdate();
						break;
					}

					++realIndex;
				}
			}
			for (int i = 0; i < characterTemplates2.size(); ++i)
			{
				if (characterTemplates2[i].isActive)
				{
					if (characterTemplates2[i].rect().leftClicked())
					{
						switchToUpdate();
						break;
					}

					++realIndex;
				}
			}
		}
		else if (state == Update)
		{
			graph.update(characters);
		}
	}

	void draw()const
	{
		if (state == Initial)
		{
			for (const auto& character : characterTemplates)
			{
				character.draw(characterNameFont, characterDeathCauseFont, character.rect().mouseOver() ? Alpha(0) : Color(0, 0, 0, 160));
			}

			if (characterHideButton)
			{
				characterHideTexture.drawAt(characterHideButton.value().center()).draw(characterHideButton.value().mouseOver() ? Alpha(0) : Color(0, 0, 0, 160));
			}

			if (showCharacter2)
			{
				for (const auto& character : characterTemplates2)
				{
					character.draw(characterNameFont, characterDeathCauseFont, character.rect().mouseOver() ? Alpha(0) : Color(0, 0, 0, 160));
				}
			}

			const int population1 = std::count_if(characterTemplates.begin(), characterTemplates.end(), [](const Character& c) {return c.isActive; });
			const int population2 = std::count_if(characterTemplates2.begin(), characterTemplates2.end(), [](const Character& c) {return c.isActive; });

			DrawBR(Scene::Rect().br(), systemFont(Format(population1 + population2, U"人村(Enterでスタート)")));
		}
		else if (state == Select)
		{
			for (const auto& character : characterTemplates)
			{
				if (character.isActive)
				{
					character.draw(characterNameFont, characterDeathCauseFont, character.rect().mouseOver() ? Alpha(0) : Color(0, 0, 0, 160));
				}
			}

			for (const auto& character : characterTemplates2)
			{
				if (character.isActive)
				{
					character.draw(characterNameFont, characterDeathCauseFont, character.rect().mouseOver() ? Alpha(0) : Color(0, 0, 0, 160));
				}
			}

			DrawBR(Scene::Rect().br(), systemFont(Format(U"自キャラを選択")));
		}
		else if (state == Update)
		{
			graph.draw(characters, characterNameFont, characterDeathCauseFont);
		}
	}

private:
	static Size LoadResolution()
	{
		//return Size(128, 128);
		return Size(100, 100);
	}

	enum State { Initial, Select, Update };
	Font characterNameFont;
	Font characterDeathCauseFont;
	Font systemFont;
	std::vector<Character> characterTemplates;
	std::vector<Character> characterTemplates2;
	std::vector<CharacterNode> characters;
	Texture characterHideTexture;
	Optional<RectF> characterHideButton;
	bool showCharacter2 = false;
	State state;
	Graph graph;
	int myselfIndex;
};

void Main()
{
	Window::SetTitle(U"人狼盤面整理ツール");
	Scene::SetBackground(Color(73, 83, 94));
	Window::Resize(1280, 720);

	auto paths = FileSystem::DirectoryContents(U"キャラクター画像1");

	auto paths2 = FileSystem::DirectoryContents(U"キャラクター画像2");

	Game game(paths, paths2);
	while (System::Update())
	{
		game.update();
		game.draw();
	}
}
