#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <string>
#include <utility>
#include <stdio.h>
#include <SFML/Graphics.hpp>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace std;
using namespace utility;
using namespace sf;

enum Commands
{
	FLIGHT = 1,
	LANDING,
	WAITING,
};

Commands cmd = Commands::WAITING;

class Pooh
{
	double mass;
	RectangleShape pooh;

public:

	Pooh(double m)
	{
		mass = m;
		pooh.setSize(Vector2f(25, 35));
		pooh.setPosition(Vector2f(630, 720));
		pooh.setFillColor(Color(73, 42, 18));
	}

	void Moving(double height)
	{
		pooh.move(Vector2f(0, -height * 10));
	}

	RectangleShape& ToDraw()
	{
		return pooh;
	}

	void Rot()
	{
		pooh.rotate(20);
	}

	double GetPoohMass()
	{
		return mass;
	}

	bool Eating(double& honeyMass)
	{
		if (honeyMass >= 0.2)
		{
			mass += 0.2;
			honeyMass -= 0.2;
			return 1;
		}
		else
		{
			mass += honeyMass;
			honeyMass = 0;
			return 0; //мёд кончился 
		}
	}
};

class World
{
	RectangleShape tree;
	RectangleShape land;
	RectangleShape hole;
	double honeyMass;

public:
	World()
	{
		tree.setSize(Vector2f(35, 720));
		tree.setPosition(675, 30);
		tree.setFillColor(Color(109, 77, 64));

		land.setSize(Vector2f(800, 50));
		land.setPosition(0, 750);
		land.setFillColor(Color(73, 159, 65));

		srand(time(NULL));
		hole.setSize(Vector2f(30, 30));
		hole.setPosition(670, (60 + rand() % 290));
		hole.setPosition(670, 250);
		hole.setFillColor(Color(50, 30, 21));

		honeyMass = (5 + rand() % 25) / 10;
	}

	std::vector<RectangleShape> ToDraw()
	{
		std::vector<RectangleShape> td;
		td.push_back(tree);
		td.push_back(land);
		td.push_back(hole);

		return td;
	}

	double GetHoleHight()
	{
		return (750 - hole.getPosition().y) * 0.1;
	}

	double GetHoneyMass()
	{
		return honeyMass;
	}

	bool Bees()
	{
		srand(time(NULL));
		return (rand() % 100 < 33) ? 1 : 0;
	}
};

class Engine
{
	double thrust;
	double maxThrust;
	double minThrust;

public:

	Engine(double min, double max, double StartThrust = 0)
	{
		thrust = StartThrust;
		minThrust = min;
		maxThrust = max;
	}

	void SetThrust(double percent)
	{
		thrust = percent * (maxThrust / 100.);
	}

	double GetThrust()
	{
		return thrust;
	}

	double GetMaxThrust()
	{
		return maxThrust;
	}

	double GetMinThrust()
	{
		return minThrust;
	}
};

class PID
{
	double pFactor;
	double iFactor;
	double dFactor;
	double dt;

	double err = 0, prevErr = 0;

	double p = 0, i = 0, d = 0;

	double output = 0;

public:

	PID(double p, double i, double d, double t)
	{
		pFactor = p;
		iFactor = i;
		dFactor = d;
		dt = t;
	}

	double Correction(double setValue, double currValue)
	{
		p = err = setValue - currValue;
		i += err * dt;
		d = (err - prevErr) / (1000 * dt);
		prevErr = err;

		output = pFactor * p + iFactor * i + dFactor * d;

		if (output > 100)
		{
			return 100;
		}
		else if (output < -100)
		{
			return -100;
		}
		else
		{
			return output;
		}
	}
};

class CS
{
	Engine* engine;
	PID* pid;
	Pooh* pooh;
	World* world;
	double height = 0, mass = 0, vel = 0, acc = 0;

	double dh = 0;
	double dv = 0;
	double dt;

	const double g = 9.810665;
	double setValue;
	double simTime = 0;
	double honeyMass;

public:

	CS(Engine* _e, PID* _pid, Pooh* _pooh, World* _world, double _dt)
	{
		engine = _e;
		pid = _pid;
		pooh = _pooh;
		world = _world;

		dt = _dt;
	}

	void SetValue(double value)
	{
		setValue = value;
	}

	void SetHoneyMass(double hm)
	{
		honeyMass = hm;
	}

	void Calculate()
	{
		mass = pooh->GetPoohMass();
		acc = engine->GetThrust() / mass;
		vel += acc * dt;
		height += vel * dt;

		engine->SetThrust(pid->Correction(setValue, height));

		simTime += dt;
		//test console logging
		system("cls");
		cout << endl << " LOGGING " << endl;
		cout << endl << " STATUS " << cmd << endl;
		cout << " Height = " << height << endl;
		cout << " Velocity = " << vel << endl;
		cout << " Acceleration = " << acc << endl;
		cout << " Power percentage = " << engine->GetThrust() << endl;
		//test console logging
	}

	float GetHeight()
	{
		return height;
	}

	float GetVelocity()
	{
		return vel;
	}

	float GetAcc()
	{
		return acc;
	}
};

class Drawing
{
	std::vector<RectangleShape> figures;
	std::vector<Text*> text;
	RectangleShape pooh;

public:

	void AddPoohToDraw(const RectangleShape& obj)
	{
		pooh = obj;
	}

	void AddToDraw(const RectangleShape& obj)
	{
		figures.push_back(obj);
	}

	void AddToDraw(const std::vector<RectangleShape>& vec)
	{
		for (int i = 0; i < vec.size(); i++)
		{
			figures.push_back(vec[i]);
		}
	}

	void AddTextToDraw(const std::vector<Text*>& t)
	{
		for (int i = 0; i < t.size(); i++)
		{
			text.push_back(t[i]);
		}
	}

	void DrawAll(RenderWindow& window)
	{
		window.draw(pooh);

		for (int i = 0; i < figures.size(); i++)
		{
			window.draw(figures[i]);
		}

		for (int i = 0; i < text.size(); i++)
		{
			window.draw(*(text[i]));
		}

		text.clear();
	}
};

class Messages
{
	CS* cs;

	Vector2f textBlockPos;

	RectangleShape background;
	Color backgroundColor;
	Vector2f backgroundSize;

	Font font;
	int fontSize;
	Color fontColor;

	vector<Text*> text;

	Text heightText;
	Text velocityText;
	Text accelerationText;
	Text statusText;

public:

	Messages(CS* CS)
	{
		cs = CS;
	}

	void SetFontSize(int size)
	{
		fontSize = size;
	}

	void SetFontColor(Color color)
	{
		fontColor = color;
	}

	void SetTextBlockPosition(Vector2f pos)
	{
		textBlockPos = pos;
	}

	void SetFont(string fontPath)
	{
		font.loadFromFile(fontPath);
	}

	void CreateTextVector()
	{
		text.clear();
		text.push_back(new Text("H: ", font, fontSize));
		text.push_back(new Text("V: ", font, fontSize));
		text.push_back(new Text("A: ", font, fontSize));
		text.push_back(new Text("Status: ", font, fontSize));
	}

	void CustomizeText()
	{
		for (int i = 0; i < text.size(); i++)
		{
			text[i]->setFillColor(fontColor);
			text[i]->setPosition(textBlockPos.x + 10, textBlockPos.y + i * (float)fontSize + 6);
		}
	}

	void UpdateTextValues()
	{
		text[0]->setString("H: " + to_string(cs->GetHeight()));
		text[1]->setString("V: " + to_string(cs->GetVelocity()));
		text[2]->setString("A: " + to_string(cs->GetAcc()));
		string status;
		switch (cmd)
		{
		case FLIGHT:
			status = "FLIGHT";
			break;
		case LANDING:
			status = "LANDING";
			break;
		case WAITING:
			status = "WAITING";
			break;
		default:
			status = "NO STATUS";
			break;
		}
		text[3]->setString("Status: " + status);
	}

	void SetBackgroundSize(Vector2f size)
	{
		backgroundSize = size;
	}

	void SetBackgroundColor(Color color)
	{
		backgroundColor = color;
	}

	void CustomizeBackground()
	{
		background.setSize(backgroundSize);
		background.setPosition(textBlockPos);
		background.setFillColor(backgroundColor);
		background.setOutlineThickness(3);
		background.setOutlineColor(Color::Black);
	}

	RectangleShape BackgroundToDraw()
	{
		return background;
	}

	vector<Text*> TextToDraw()
	{
		return text;
	}
};

/*
 * пример curl запроса, на место # написать номер команды
 * 
 * FLIGHT 1
 * LANDING 2
 * WAITING 3
 * 
 * curl -X POST -H "Content-Type:application/json" -d \"#\" http://localhost
 */

void POST(http_request request)
{
	cout << "Recived POST request" << endl;
	try
	{
		auto extracted = request.extract_json();
		auto jvalue = extracted.get();
		if (!jvalue.is_null())
		{
			auto str = jvalue.as_string();
			cmd = static_cast<Commands>(atoi(conversions::to_utf8string(str).c_str()));
		}
	}
	catch (const http_exception& e)
	{
		cout << "ERROR: " << e.what() << endl;
	}
	string answer;
	answer = " Status set: " + to_string(((int)cmd));
	request.reply(status_codes::OK, json::value(conversions::to_string_t(answer)));
}

void Server(http_listener& listener)
{
	listener.open().then([&listener]() {
		cout << "\nWAITING FOR REQUEST\n" << endl;
		}).wait();
		while (1);
}

int main()
{
	uri uri;
	http_listener listener(uri.encode_uri(conversions::to_string_t("http://localhost/")));
	listener.support(methods::POST, POST);

	RenderWindow window(VideoMode(800, 800), "Pooh", Style::Close);

	Drawing drawing;

	Pooh pooh(15);

	World world;

	Engine e(-500, 500);

	PID pid(0.2, 0.02, 0.12, 0.016);

	CS cs(&e, &pid, &pooh, &world, 0.016);

	Messages messages(&cs);
	messages.SetFont("C:\\Windows\\Fonts\\consola.ttf");
	messages.SetFontColor(Color::Black);
	messages.SetFontSize(20);
	messages.SetTextBlockPosition(Vector2f(50, 50));

	messages.SetBackgroundSize(Vector2f(180, 100));
	messages.SetBackgroundColor(Color::White);

	messages.CreateTextVector();
	messages.CustomizeText();
	messages.CustomizeBackground();

	cmd = Commands::WAITING;

	std::thread serverThread(Server, std::ref(listener));

	drawing.AddToDraw(world.ToDraw());

	double overallTime = 0;

	while (window.isOpen())
	{
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::Closed) 
			{
				serverThread.detach();
				window.close();
			}
		}

		switch (cmd)
		{
		case FLIGHT:
		{
			cs.SetValue(world.GetHoleHight());
			cs.SetHoneyMass(world.GetHoneyMass());

			cs.Calculate();
			pooh.Moving(cs.GetHeight());

			break;
		}
		case LANDING:
		{
			cs.SetValue(0);
			cs.Calculate();
			pooh.Moving(cs.GetHeight());

			break;
		}
		case WAITING:
		{
			std::system("cls");
			std::cout << "POOH IS WAITING" << std::endl;

			break;
		}
		default:
			std::cout << "\nINVALID COMMAND" << std::endl;
			break;
		}

		window.clear(Color(120, 219, 226, 255));
		messages.UpdateTextValues();
		drawing.AddPoohToDraw(pooh.ToDraw());
		drawing.AddToDraw(messages.BackgroundToDraw());
		drawing.AddTextToDraw(messages.TextToDraw());
		drawing.DrawAll(window);

		window.display();
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	return 0;
}
