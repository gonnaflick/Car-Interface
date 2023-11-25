import pygame
import socket
import threading

# Configuración inicial de Pygame
pygame.init()

# Configuración para la conexión TCP
ip = '192.168.1.23'
port = 23
estados = {
    "izquierda": False,
    "derecha": False,
    "ambas": False,
    "cortas": False,
    "largas": False,
}

# Función para enviar comandos TCP
def send_command(command):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.connect((ip, port))
            s.sendall(command.encode() + b'\n')
            data = s.recv(1024)
            print('Recibido:', repr(data))
        except Exception as e:
            print(f"Error al conectar o enviar datos: {e}")

def toggle_button_state(button):
    global estados

    if estados[button]:
        send_command('apagar' if button in ['izquierda', 'derecha', 'ambas'] else 'off')
        estados[button] = False
    else:
        if button == "ambas":
            send_command('apagar')
            pygame.time.wait(500)
        estados[button] = True
        send_command(button)
        
    if button in ['cortas', 'largas']:
        for key in ['cortas', 'largas']:
            if key != button:
                estados[key] = False

    if button in ['izquierda', 'derecha', 'ambas']:
        for key in ['izquierda', 'derecha', 'ambas']:
            if key != button:
                estados[key] = False

size = (1024, 600)  # Tamaño de la tableta
screen = pygame.display.set_mode(size)
pygame.display.set_caption("Controlador ESP32")

# Colores al estilo GitHub Dark
DARK_BACKGROUND = (13, 17, 23)
DARKER_BACKGROUND = (36, 41, 46)
GREY_TEXT = (140, 140, 140)
SOFT_BLUE = (88, 166, 255)

font = pygame.font.Font(None, 36)

# Cálculos para la cuadrícula
button_width = size[0] // 3 - 60
button_height = size[1] // 3 - 60
button_size = (button_width, button_height)
button_padding = 30

# Configurar los botones en una cuadrícula
buttons = {
    "izquierda": pygame.Rect(button_padding, button_padding, *button_size),
    "derecha": pygame.Rect(size[0] // 3 + button_padding, button_padding, *button_size),
    "ambas": pygame.Rect(size[0] * 2 // 3 + button_padding, button_padding, *button_size),
    "cortas": pygame.Rect(button_padding, size[1] // 3 + button_padding, *button_size),
    "largas": pygame.Rect(size[0] // 3 + button_padding, size[1] // 3 + button_padding, *button_size),
}

def draw_buttons():
    screen.fill(DARK_BACKGROUND)  # Fondo oscuro
    for text, btn in buttons.items():
        pygame.draw.rect(screen, DARKER_BACKGROUND if estados.get(text, False) else DARK_BACKGROUND, btn, border_radius=15)
        btn_text = font.render(text.capitalize(), True, GREY_TEXT if estados.get(text, False) else SOFT_BLUE)
        text_rect = btn_text.get_rect(center=btn.center)
        screen.blit(btn_text, text_rect)
        
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
                print(f"{signal}")
                send_command('apagar')
                estados['izquierda'] = False
                estados['derecha'] = False
            # Ignorar mensajes que comienzan con 'd'
            elif signal.startswith('d'):
                continue  # Esto ignorará el resto del código en el bucle y volverá al inicio

# Iniciar el servidor UDP en un hilo separado
thread_udp = threading.Thread(target=udp_server, daemon=True)
thread_udp.start()

done = False
clock = pygame.time.Clock()
while not done:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            done = True
        elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            for text, btn in buttons.items():
                if btn.collidepoint(event.pos):
                    toggle_button_state(text)

    draw_buttons()
    pygame.display.flip()
    clock.tick(60)

pygame.quit()