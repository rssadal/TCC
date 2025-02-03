CREATE DATABASE estufa;

CREATE TABLE planta (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    tempo_luminosidade INT NOT NULL COMMENT 'Tempo de exposição à luz em horas',
    temperatura_min DECIMAL(5,2) NOT NULL COMMENT 'Temperatura mínima ideal (°C)',
    temperatura_max DECIMAL(5,2) NOT NULL COMMENT 'Temperatura máxima ideal (°C)',
    umidade_solo_min DECIMAL(5,2) NOT NULL COMMENT 'Umidade mínima ideal do solo (%)',
    umidade_solo_max DECIMAL(5,2) NOT NULL COMMENT 'Umidade máxima ideal do solo (%)'
);

INSERT INTO plantas (name, tempo_luminosidade, temperatura_min, temperatura_max, umidade_solo_min, umidade_solo_max) VALUES
('Erva-mate', 12, 20.00, 35.00, 60.00, 80.00),
('Tomate', 8, 18.00, 30.00, 60.00, 80.00),
('Alface', 10, 10.00, 25.00, 60.00, 90.00),
('Morango', 8, 15.00, 25.00, 60.00, 85.00);
