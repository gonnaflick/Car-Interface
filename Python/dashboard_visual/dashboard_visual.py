import pygame
import sys
import math
import time
import socket
import threading

# Inicializar Pygame
pygame.init()

# Declaramos las variables globalmente
direction_left_state = 0
direction_right_state = 0
light_state = 0
pinLucesLargas = 0
pinLucesCortas = 0
belt_state = 0
speedometer = 0
counter = 0
light_state = 0
fuel = 100
fuel_increment = 0

# Ángulo inicial del medidor
angle_start = -135 # Cambiado para que el ángulo inicial sea 0 grados
angle_start_fuel = -90

# Ángulo máximo del medidor
angle_max = 150  # Cambiado para que el ángulo máximo sea 225 grados


# Contadores para el botón BELT
belt_counter = 0
belt_delay = 5  # Ajustar según sea necesario


def parse_signal(signal):
    global direction_left_state, direction_right_state, pinLucesCortas, pinLucesLargas, belt_state, speedometer, counter, light_state
    # Split the signal string into a list of values
    values = signal.split()
    
    # Check if the signal starts with 'd' and has the correct number of values
    if values[0] == 'd' and len(values) == 8:
        # Assign the values to the variables by unpacking the list
        _, pinFocoIzquierda, pinFocoDerecha, pinLucesCortas, pinLucesLargas, \
        belt_state, speedometer, counter = values
        
        # Convert string values to integers
        direction_left_state = int(pinFocoDerecha)
        direction_right_state = int(pinFocoIzquierda)
        pinLucesCortas = int(pinLucesCortas)
        pinLucesLargas = int(pinLucesLargas)
        belt_state = int(belt_state)
        speedometer = int(speedometer)
        counter = int(counter)
        
        if (pinLucesCortas == 0 & pinLucesLargas == 0):
            light_state = 0            
        elif (pinLucesCortas == 1 & pinLucesLargas == 1):
            light_state = 2
        else:
            light_state = 1
        
        return (direction_right_state, direction_left_state, light_state,
                belt_state, speedometer, counter)
    else:
        return None
    
# Función para el servidor UDP
def udp_server():
    host = "0.0.0.0"
    port = 15000
    
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server_socket:
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((host, port))
        print(f"Escuchando en {host}:{port}")

        while True:
            data, addr = server_socket.recvfrom(1024)
            signal = data.decode('utf-8')
            # Verificar si el mensaje comienza con 's'
            if signal.startswith('s'):
                continue
            # Ignorar mensajes que comienzan con 'd'
            elif signal.startswith('d'):
                parse_signal(signal)
                print(f"Variables: {direction_left_state}, {direction_right_state}, {light_state}, {belt_state}, {speedometer}, {counter}")



# Iniciar el servidor UDP en un hilo separado
thread_udp = threading.Thread(target=udp_server, daemon=True)
thread_udp.start()
# Configuración del display
width, height = 1200, 880
screen = pygame.display.set_mode((width, height))
pygame.display.set_caption("Dashboard")

# Definir el centro y el radio de los medidores
center_x_speed = width // 4
center_x_fuel = 3 * width // 4
center_y = height // 2

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

# Función para dibujar los medidores
def draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state):
    # Limpiar la pantalla
    screen.fill((0, 0, 0))

    # Dibujar el sprite del suelo
    screen.blit(low_ground, (0, 0))

    # Dibujar el medidor de velocidad
    screen.blit(background_speed, (center_x_speed - background_speed.get_width() // 2, center_y - background_speed.get_height() // 2))
    angle_speed = (angle_start + (speedometer / 200) * angle_max) % 360  # Grados para el medidor de velocidad
    rotated_needle_speed = pygame.transform.rotate(needle_speed, -angle_speed)
    needle_rect_speed = rotated_needle_speed.get_rect(center=(center_x_speed, center_y))
    screen.blit(rotated_needle_speed, needle_rect_speed.topleft)

    # Mostrar la velocidad actual
    font = pygame.font.Font(None, 36)
    speed_text = font.render(f"Velocidad: {speedometer} km/h", True, (255, 255, 255))
    screen.blit(speed_text, (10, 10))

    # Dibujar el medidor de combustible
    screen.blit(background_fuel, (center_x_fuel - background_fuel.get_width() // 2, center_y - background_fuel.get_height() // 2))
    angle_fuel = (angle_start_fuel + (fuel / 100) * 225)  # Grados para el medidor de combustible
    rotated_needle_fuel = pygame.transform.rotate(needle_fuel, -angle_fuel)
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


# Establecer estados iniciales
draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state)

# Bucle principal
running = True
clock = pygame.time.Clock()

# Esta es la tasa a la que el combustible se va a consumir
fuel_consumption_rate = 0.1  # Puedes ajustar este valor como sea necesario

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    if speedometer > 0:
        # Asumiendo que 'speedometer' es un valor entre 0 y 200, ajusta el consumo de combustible en consecuencia
        # El combustible se consumirá más rápido a mayores velocidades
        fuel -= fuel_consumption_rate * (speedometer / 200)
        fuel = max(fuel, 0)  # Asegurarse de que el combustible no sea negativo

    # Controlar la velocidad del bucle y actualizar la pantalla
    clock.tick(30)
    draw_dashboard(speedometer, fuel, belt_state, direction_left_state, direction_right_state, light_state)


# Limpiar y cerrar el programa al salir
pygame.quit()
sys.exit()