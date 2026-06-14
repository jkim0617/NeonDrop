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
    int id = 0;

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
    int id = 0;
    
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

bool circleIntersectsLaser(const sf::CircleShape& player, float radius, const Laser& laser)
{
    if (!laser.active)
        return false;

    // Convert player center from world space into laser's local space
    sf::Vector2f localCenter =
        laser.shape.getTransform().getInverse().transformPoint(player.getPosition());

    sf::Vector2f laserSize = laser.shape.getSize();

    // Closest point on the laser rectangle to the player's center
    float closestX = std::clamp(localCenter.x, 0.0f, laserSize.x);
    float closestY = std::clamp(localCenter.y, 0.0f, laserSize.y);

    float dx = localCenter.x - closestX;
    float dy = localCenter.y - closestY;

    return (dx * dx + dy * dy) <= (radius * radius);
}

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

    int laserIdCounter = 1;

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

            event.id = laserIdCounter++;

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
    
    const int maxHealth = 20;
    int playerHealth = maxHealth;

    bool gameOver = false;

    float damageCooldown = 0.0f;
    const float damageCooldownDuration = 0.75f;

    // Beat / strobe system
    LevelData level = loadLevel("levels/laserbeam.txt");

    float mapBPM = level.bpm;
    float beatInterval = 60.0f / mapBPM;

    // Debug start point
    float debugStartBeat = 00.0f; // Change this to any beat you want
    float debugStartTime = debugStartBeat * beatInterval;

    std::vector<LaserEvent> laserEvents = level.laserEvents;
    std::vector<Laser> lasers;

    for (auto& event : laserEvents)
    {
        if (event.activeBeat < debugStartBeat)
        {
            event.spawned = true;
        }
    }

    // float songTime = 0.0f;
    float songTime = debugStartTime;
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

    if (!music.openFromFile("assets/music/Ray Volpe - Laserbeam.mp3"))
    {
        std::cerr << "Failed to load music file." << std::endl;
    }

    music.setVolume(10.0f);  // 0 to 100
    music.setLooping(false); // true if you want it to repeat

    music.setPlayingOffset(sf::seconds(debugStartTime));   

    music.play();

    sf::Font debugFont;

    if (!debugFont.openFromFile("assets/fonts/debug.ttf"))
    {
        std::cerr << "Failed to load debug font." << std::endl;
        return 1;
    }

    sf::Text debugText(debugFont);
    debugText.setCharacterSize(22);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition({20.0f, 20.0f});

    sf::Text gameOverText(debugFont);
    gameOverText.setCharacterSize(64);
    gameOverText.setFillColor(sf::Color::White);
    gameOverText.setString("GAME OVER");

    sf::FloatRect gameOverBounds = gameOverText.getLocalBounds();
    gameOverText.setOrigin({
        gameOverBounds.position.x + gameOverBounds.size.x / 2.0f,
        gameOverBounds.position.y + gameOverBounds.size.y / 2.0f
    });
    gameOverText.setPosition({windowWidth / 2.0f, windowHeight / 2.0f - 60.0f});


    sf::Text restartText(debugFont);
    restartText.setCharacterSize(28);
    restartText.setFillColor(sf::Color::White);
    restartText.setString("Press R to restart  |  Esc to quit");

    sf::FloatRect restartBounds = restartText.getLocalBounds();
    restartText.setOrigin({
        restartBounds.position.x + restartBounds.size.x / 2.0f,
        restartBounds.position.y + restartBounds.size.y / 2.0f
    });
    restartText.setPosition({windowWidth / 2.0f, windowHeight / 2.0f + 20.0f});

    int lastSpawnedLaserId = 0;

    sf::Clock clock;

    auto restartGame = [&]()
    {
        playerHealth = maxHealth;
        damageCooldown = 0.0f;
        gameOver = false;

        player.setPosition({windowWidth / 2.0f, windowHeight / 2.0f});
        player.setFillColor(sf::Color::Red);

        lasers.clear();

        for (auto& event : laserEvents)
        {
            event.spawned = event.activeBeat < debugStartBeat;
        }

        songTime = debugStartTime;
        lastBeatIndex = -1;
        beatPulse = 0.0f;
        lastSpawnedLaserId = 0;

        music.stop();
        music.setPlayingOffset(sf::seconds(debugStartTime));
        music.play();

        clock.restart();
    };

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();
        songTime = music.getPlayingOffset().asSeconds();

        int currentBeatIndex = static_cast<int>(std::floor(songTime / beatInterval));

        // Every beat, trigger a strobe pulse
        if (currentBeatIndex != lastBeatIndex)
        {
            lastBeatIndex = currentBeatIndex;
            beatPulse = 1.0f;
        }

        float currentBeatFloat = songTime / beatInterval;

        debugText.setString(
            "Beat: " + std::to_string(static_cast<int>(currentBeatFloat)) +
            "\nLast Laser: " + std::to_string(lastSpawnedLaserId) +
            "\nActive Lasers: " + std::to_string(lasers.size()) +
            "\nTotal Lasers: " + std::to_string(laserEvents.size()) +
            "\nHealth: " + std::to_string(playerHealth)
        );

        for (auto &event : laserEvents)
        {
            float spawnBeat = event.activeBeat - event.warningBeats;

            if (!event.spawned && currentBeatFloat >= spawnBeat)
            {
                event.spawned = true;

                Laser laser;

                laser.id = event.id;
                lastSpawnedLaserId = event.id;

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

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }

                if (gameOver && keyPressed->code == sf::Keyboard::Key::R)
                {
                    restartGame();
                }
            }
        }

        if (gameOver)
        {
            window.clear(sf::Color(20, 0, 0));

            window.draw(gameOverText);
            window.draw(restartText);

            window.display();

            continue;
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

        // Damage cooldown timer
        if (damageCooldown > 0.0f)
        {
            damageCooldown -= deltaTime;
        }

        // Check active laser collision
        bool playerTouchingLaser = false;

        for (const auto& laser : lasers)
        {
            if (circleIntersectsLaser(player, playerRadius, laser))
            {
                playerTouchingLaser = true;
                break;
            }
        }

        if (playerTouchingLaser && damageCooldown <= 0.0f)
        {
            playerHealth--;
            damageCooldown = damageCooldownDuration;

            std::cout << "Player hit! Health: " << playerHealth << std::endl;

            if (playerHealth <= 0)
            {
                std::cout << "Game Over!" << std::endl;

                gameOver = true;
                music.pause();
            }
        }

        // Visual hit feedback
        if (damageCooldown > 0.0f)
        {
            player.setFillColor(sf::Color::White);
        }
        else
        {
            player.setFillColor(sf::Color::Red);
        }

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

        window.draw(debugText);

        window.display();
    }

    return 0;
}