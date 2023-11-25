import pygame
import sys
import math
import time
import socket
import select

# Initialize Pygame and Socket
pygame.init()

# Set up non-blocking UDP server
host, port = "0.0.0.0", 15000
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind((host, port))
server_socket.setblocking(False)

# Configuración del display
width, height = 1200, 880
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("Dashboard")

# Cargar imágenes de fondo y aguja
background_speed = pygame.image.load("background_speed.png")
needle_speed = pygame.image.load("needle_speed.png")
background_fuel = pygame.image.load("background_fuel.png")
needle_fuel = pygame.image.load("needle_fuel.png")
low_ground = pygame.image.load("lowground.png")

# Cargar imágenes para los botones
belt_danger = pygame.image.load("belt_danger.png")
belt_red = pygame.image.load("belt_red.png")
belt_okey = pygame.image.load("belt_okey.png")

direction_left_off = pygame.image.load("direction_left_off.png")
direction_left_on = pygame.image.load("direction_left_on.png")
direction_right_off = pygame.image.load("direction_right_off.png")
direction_right_on = pygame.image.load("direction_right_on.png")

light_off = pygame.image.load("light_off.png")
light_low = pygame.image.load("light_low.png")
light_high = pygame.image.load("light_high.png")

# Definir el centro y el radio de los medidores
center_x_speed = width // 4
center_x_fuel = 3 * width // 4
center_y = height // 2

# Velocidad y combustible iniciales
speedometer = 0
fuel = 100

# Estados de los botones
belt_state = 0
direction_right_state = 0
direction_left_state = 0
light_state = 0

# Ángulo inicial del medidor
angle_start = 0  # Cambiado para que el ángulo inicial sea 0 grados

# Ángulo máximo del medidor
angle_max = 225  # Cambiado para que el ángulo máximo sea 225 grados

# Variables para el movimiento de los medidores
speedometer_increment = 0
fuel_increment = 0
speedometer_direction = 1  # Dirección: 1 para incremento, -1 para decremento
fuel_direction = -1  # Dirección: -1 para decremento, 1 para incremento

# Contadores para el botón BELT
belt_counter = 0
belt_delay = 5  # Ajustar según sea necesario

# Función para dibujar los medidores
def draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state):
    # Limpiar la pantalla
    screen.fill((0, 0, 0))

    # Dibujar el sprite del suelo
    screen.blit(low_ground, (0, 0))

    # Dibujar el medidor de velocidad
    screen.blit(background_speed, (center_x_speed - background_speed.get_width() // 2, center_y - background_speed.get_height() // 2))
    angle_speed = math.radians(angle_start + (speedometer / 200) * angle_max)
    rotated_needle_speed = pygame.transform.rotate(needle_speed, -angle_speed * 1.8)
    needle_rect_speed = rotated_needle_speed.get_rect(center=(center_x_speed, center_y))
    screen.blit(rotated_needle_speed, needle_rect_speed.topleft)

    # Mostrar la velocidad actual
    font = pygame.font.Font(None, 36)
    speed_text = font.render(f"Velocidad: {speedometer} km/h", True, (255, 255, 255))
    screen.blit(speed_text, (10, 10))

    # Dibujar el medidor de combustible
    screen.blit(background_fuel, (center_x_fuel - background_fuel.get_width() // 2, center_y - background_fuel.get_height() // 2))
    angle_fuel = math.radians(angle_start - (fuel / 100) * angle_max)
    rotated_needle_fuel = pygame.transform.rotate(needle_fuel, -angle_fuel * 1.8)
    needle_rect_fuel = rotated_needle_fuel.get_rect(center=(center_x_fuel, center_y))
    screen.blit(rotated_needle_fuel, needle_rect_fuel.topleft)

    # Mostrar el nivel de combustible
    fuel_text = font.render(f"Combustible: {fuel}%", True, (255, 255, 255))
    screen.blit(fuel_text, (width // 2 + 10, 10))

    # Dibujar los botones
    # BOTON DEL CINTURON
    position_belt = (2 * width // 4 - 25, center_y - 300)
    if belt_state == 0:
        screen.blit(belt_okey, position_belt)
    elif belt_state == 1:
        screen.blit(belt_red, position_belt)
    elif belt_state == 2:
        screen.blit(belt_danger, position_belt)

    # DIRECCIONAL IZQUIERDA
    position_left = (center_x_fuel - 120, center_y - 300)
    if direction_left_state == 0:
        screen.blit(direction_left_off, position_left)
    elif direction_left_state == 1:
        screen.blit(direction_left_on, position_left)

    # DIRECCIONAL DERECHA
    position_right = (center_x_speed + 50, center_y - 300)
    if direction_right_state == 0:
        screen.blit(direction_right_off, position_right)
    elif direction_right_state == 1:
        screen.blit(direction_right_on, position_right)

    # BOTON DE LUCES
    position_light = (2 * width // 4 - 80, center_y - 200)
    if light_state == 0:
        screen.blit(light_off, position_light)
    elif light_state == 1:
        screen.blit(light_low, position_light)
    elif light_state == 2:
        screen.blit(light_high, position_light)

    # Actualizar la pantalla
    pygame.display.flip()

def update_dashboard_from_udp(data):
    # Parse the incoming data and update Pygame dashboard variables
    pass

# Establecer estados iniciales
draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state)

# Bucle principal
running = True
clock = pygame.time.Clock()

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    keys = pygame.key.get_pressed()

    # Check for incoming UDP data
    ready = select.select([server_socket], [], [], 0.1)[0]
    if ready:
        data, addr = server_socket.recvfrom(1024)
        update_dashboard_from_udp(data)

    # Manejo de estados de botones
    # BOTON DEL CINTURON
    if keys[pygame.K_b]:
        belt_counter += 1
        if belt_counter >= belt_delay:
            belt_counter = 0
            belt_state = (belt_state + 1) % 3

    direction_right_state = keys[pygame.K_RIGHT]
    direction_left_state = keys[pygame.K_LEFT]

    # BOTON DE LUCES
    if keys[pygame.K_l]:
        time.sleep(0.2)  # Agregar un ligero delay en el cambio de light
        light_state = (light_state + 1) % 3

    # Actualizar el movimiento del medidor de velocidad
    if keys[pygame.K_UP]:
        speedometer_increment += 1
        speedometer_direction = 1
    else:
        speedometer_increment -= 1
        speedometer_direction = -1

    # Actualizar el movimiento del medidor de combustible
    if keys[pygame.K_DOWN]:
        fuel_increment += 1
        fuel_direction = 1
    else:
        fuel_increment -= 1
        fuel_direction = -1

    # Aplicar incrementos y direcciones
    speedometer = (speedometer + speedometer_increment * speedometer_direction) % 200
    fuel = max(min(fuel + fuel_increment * fuel_direction, 100), 0)

    # Controlar la velocidad del bucle
    clock.tick(30)

    # Dibujar el tablero con los nuevos estados
    draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state)

# Limpiar y cerrar el programa al salir
server_socket.close()
pygame.quit()
sys.exit()
