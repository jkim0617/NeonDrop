#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <algorithm>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <iostream>

struct Laser
{
    sf::RectangleShape shape;
    float age = 0.0f;
    float warningTime = 0.4f;
    float activeTime = 0.2f;
    bool active = false;

    sf::Color warningColor;
    sf::Color activeColor;
};

struct LaserEvent
{
    float activeBeat = 0.0f;

    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;

    float length = 2000.0f;
    float thickness = 18.0f;

    float warningBeats = 1.0f;
    float activeBeats = 0.5f;

    sf::Color color = sf::Color(0, 255, 0, 220);

    bool spawned = false;
};

struct LevelData
{
    float bpm = 150.0f;
    std::vector<LaserEvent> laserEvents;
};

LevelData loadLevel(const std::string &filename)
{
    LevelData level;

    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Failed to open level file: " << filename << std::endl;
        return level;
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream iss(line);

        std::string command;
        iss >> command;

        if (command == "BPM")
        {
            iss >> level.bpm;
        }
        else if (command == "LASER")
        {
            LaserEvent event;

            int r = 255;
            int g = 255;
            int b = 255;

            iss >> event.activeBeat >> event.x >> event.y >> event.angle;

            level.laserEvents.push_back(event);
        }
    }

    return level;
}

int main()
{
    sf::RenderWindow window(
        sf::VideoMode({1280, 720}),
        "Neon Drop");

    window.setFramerateLimit(60);

    const float windowWidth = 1280.0f;
    const float windowHeight = 720.0f;

    // Player
    const float playerRadius = 25.0f;
    const float playerSpeed = 350.0f;

    sf::CircleShape player(playerRadius);
    player.setOrigin({playerRadius, playerRadius});
    player.setPosition({windowWidth / 2.0f, windowHeight / 2.0f});
    player.setFillColor(sf::Color::Red);

    // Beat / strobe system
    LevelData level = loadLevel("levels/laserbeam.txt");

    float mapBPM = level.bpm;
    float beatInterval = 60.0f / mapBPM;

    std::vector<LaserEvent> laserEvents = level.laserEvents;
    std::vector<Laser> lasers;

    float songTime = 0.0f;
    int lastBeatIndex = -1;
    float beatPulse = 0.0f;

    // Test obstacle lasers - visual only for now

    // Vertical laser
    sf::RectangleShape verticalLaser({8.0f, windowHeight});
    verticalLaser.setOrigin({9.0f, windowHeight / 2.0f});
    verticalLaser.setPosition({320.0f, windowHeight / 2.0f});
    // verticalLaser.setFillColor(sf::Color(255, 0, 255, 170));
    verticalLaser.setFillColor(sf::Color(0, 255, 0, 170));

    // Horizontal laser
    sf::RectangleShape horizontalLaser({windowWidth, 8.0f});
    horizontalLaser.setOrigin({windowWidth / 2.0f, 9.0f});
    horizontalLaser.setPosition({windowWidth / 2.0f, 520.0f});
    // horizontalLaser.setFillColor(sf::Color(0, 255, 255, 170));
    horizontalLaser.setFillColor(sf::Color(0, 255, 0, 170));

    // Diagonal laser
    sf::RectangleShape diagonalLaser({1500.0f, 8.0f});
    diagonalLaser.setOrigin({750.0f, 7.0f});
    diagonalLaser.setPosition({windowWidth / 2.0f, windowHeight / 2.0f});
    diagonalLaser.setRotation(sf::degrees(35.0f));
    // diagonalLaser.setFillColor(sf::Color(255, 80, 80, 160));
    diagonalLaser.setFillColor(sf::Color(0, 255, 0, 170));

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_real_distribution<float> verticalLaserX(150.0f, windowWidth - 150.0f);
    std::uniform_real_distribution<float> horizontalLaserY(150.0f, windowHeight - 150.0f);

    // MUSIC ADDED FIX ME
    sf::Music music;

    if (!music.openFromFile("assets/music/Ray Volpe - Laserbeam.ogg"))
    {
        std::cerr << "Failed to load music file." << std::endl;
    }

    music.setVolume(70.0f);  // 0 to 100
    music.setLooping(false); // true if you want it to repeat
    music.play();

    sf::Clock clock;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();
        songTime += deltaTime;

        int currentBeatIndex = static_cast<int>(std::floor(songTime / beatInterval));

        // Every beat, trigger a strobe pulse
        if (currentBeatIndex != lastBeatIndex)
        {
            lastBeatIndex = currentBeatIndex;
            beatPulse = 1.0f;
        }

        float currentBeatFloat = songTime / beatInterval;

        for (auto &event : laserEvents)
        {
            float spawnBeat = event.activeBeat - event.warningBeats;

            if (!event.spawned && currentBeatFloat >= spawnBeat)
            {
                event.spawned = true;

                Laser laser;

                laser.warningTime = event.warningBeats * beatInterval;
                laser.activeTime = event.activeBeats * beatInterval;

                laser.shape.setSize({event.length, event.thickness});
                laser.shape.setOrigin({event.length / 2.0f, event.thickness / 2.0f});
                laser.shape.setPosition({event.x, event.y});
                laser.shape.setRotation(sf::degrees(event.angle));

                laser.warningColor = sf::Color(
                    event.color.r,
                    event.color.g,
                    event.color.b,
                    70);

                laser.activeColor = event.color;

                laser.shape.setFillColor(laser.warningColor);

                lasers.push_back(laser);
            }
        }

        while (auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        // Player movement
        sf::Vector2f movement(0.0f, 0.0f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            movement.y -= 1.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            movement.y += 1.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            movement.x -= 1.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            movement.x += 1.0f;

        float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);

        if (length != 0.0f)
        {
            movement.x /= length;
            movement.y /= length;
        }

        sf::Vector2f position = player.getPosition();

        position.x += movement.x * playerSpeed * deltaTime;
        position.y += movement.y * playerSpeed * deltaTime;

        position.x = std::clamp(position.x, playerRadius, windowWidth - playerRadius);
        position.y = std::clamp(position.y, playerRadius, windowHeight - playerRadius);

        player.setPosition(position);

        for (auto &laser : lasers)
        {
            laser.age += deltaTime;

            if (laser.age < laser.warningTime)
            {
                laser.active = false;
                laser.shape.setFillColor(laser.warningColor);
            }
            else
            {
                laser.active = true;
                laser.shape.setFillColor(laser.activeColor);
            }
        }

        lasers.erase(
            std::remove_if(
                lasers.begin(),
                lasers.end(),
                [](const Laser &laser)
                {
                    return laser.age > laser.warningTime + laser.activeTime;
                }),
            lasers.end());

        // Decay strobe pulse
        beatPulse -= deltaTime * 3.5f;

        if (beatPulse < 0.0f)
            beatPulse = 0.0f;

        // Background strobe
        int pulse = static_cast<int>(30 + beatPulse * 120);
        window.clear(sf::Color(pulse, 5, 5));

        // Draw pulsing ring around player
        if (beatPulse > 0.0f)
        {
            float ringRadius = playerRadius + beatPulse * 45.0f;

            sf::CircleShape pulseRing(ringRadius);
            pulseRing.setOrigin({ringRadius, ringRadius});
            pulseRing.setPosition(player.getPosition());
            pulseRing.setFillColor(sf::Color::Transparent);
            pulseRing.setOutlineThickness(3.0f);
            pulseRing.setOutlineColor(sf::Color::Red);

            window.draw(pulseRing);
        }

        for (const auto &laser : lasers)
        {
            window.draw(laser.shape);
        }

        window.draw(player);

        window.display();
    }

    return 0;
}