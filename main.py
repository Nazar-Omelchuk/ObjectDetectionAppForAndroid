import urllib.request

import cv2
import numpy as np
from kivy.clock import Clock
from kivy.core.image import Texture
from kivy.lang import Builder
from kivymd.app import MDApp

# Рядок Kivy, що визначає макет
kv = """
BoxLayout:
    orientation: 'vertical'
    Image:
        id: img
"""


class ObjectDetectionApp(MDApp):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.url = 'http://192.168.137.96/cam-hi.jpg'
        self.whT = 320
        self.confThreshold = 0.5
        self.nmsThreshold = 0.3
        self.classesfile = 'coco.names'
        self.classNames = []
        with open(self.classesfile, 'rt') as f:
            self.classNames = f.read().rstrip('\n').split('\n')

        self.modelConfig = 'yolov3.cfg'
        self.modelWeights = 'yolov3.weights'
        self.net = cv2.dnn.readNetFromDarknet(self.modelConfig, self.modelWeights)
        self.net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
        self.net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

    def build(self):
        self.root = Builder.load_string(kv)
        Clock.schedule_interval(self.update_image, 1.0 / 60.0)  # Розкладка методу update_image() на виклик кожні 1/60 секунди
        return self.root

    def update_image(self, dt):
        img_resp = urllib.request.urlopen(self.url)  # Відкриття URL та отримання відповіді зображення
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)  # Конвертація відповіді зображення в масив NumPy
        im = cv2.imdecode(imgnp, -1)  # Декодування зображення за допомогою OpenCV

        # Створення блоба з зображення для подальшого введення у нейромережу
        blob = cv2.dnn.blobFromImage(im, 1 / 255, (self.whT, self.whT), [0, 0, 0], 1, crop=False)
        self.net.setInput(blob)

        # Отримання імен вихідних шарів та запуск прямого проходу через мережу
        layernames = self.net.getLayerNames()
        outputNames = [layernames[i - 1] for i in self.net.getUnconnectedOutLayers()]
        outputs = self.net.forward(outputNames)

        # Знаходження об'єктів на зображенні за допомогою вихідних даних від мережі
        self.find_object(outputs, im)

        # Оновлення відображення зображення у Kivy
        image_widget = self.root.ids.img
        buf = cv2.flip(im, 0).tostring()
        texture = Texture.create(size=(im.shape[1], im.shape[0]), colorfmt='bgr')
        texture.blit_buffer(buf, colorfmt='bgr', bufferfmt='ubyte')
        image_widget.texture = texture

    def find_object(self, outputs, im):
        hT, wT, cT = im.shape
        bbox = []
        classIds = []
        confs = []

        # Проходження через вихідні дані мережі та знаходження об'єктів з високою достовірністю
        for output in outputs:
            for det in output:
                scores = det[5:]
                classId = np.argmax(scores)
                confidence = scores[classId]
                if confidence > self.confThreshold:
                    w, h = int(det[2] * wT), int(det[3] * hT)
                    x, y = int((det[0] * wT) - w / 2), int((det[1] * hT) - h / 2)
                    bbox.append([x, y, w, h])
                    classIds.append(classId)
                    confs.append(float(confidence))

        # Застосування алгоритму non-maximum suppression для видалення прямокутників, що перекриваються
        indices = cv2.dnn.NMSBoxes(bbox, confs, self.confThreshold, self.nmsThreshold)

        # Малювання прямокутників обмежень та міток на зображенні
        for i in indices:
            box = bbox[i]
            x, y, w, h = box[0], box[1], box[2], box[3]
            cv2.rectangle(im, (x, y), (x + w, y + h), (255, 0, 255), 2)
            cv2.putText(im, f'{self.classNames[classIds[i]].upper()} {int(confs[i] * 100)}%', (x, y - 10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)


if __name__ == '__main__':
    ObjectDetectionApp().run()  # Запуск додатку Kivy
