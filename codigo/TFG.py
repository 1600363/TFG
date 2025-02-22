#!/usr/bin/env python
# coding: utf-8

# # TFG

# In[3]:


import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn import preprocessing

import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.callbacks import EarlyStopping

import joblib


# Importamos dataset

# In[6]:


dataset = pd.read_csv(r"D:\TFG\datos\BdwifisV2.csv", encoding='latin1')
test_dataset = pd.read_csv(r"D:\TFG\datos\test.csv", encoding='latin1')
test2_dataset = pd.read_csv(r"D:\TFG\datos\test2.csv", encoding='latin1')
test3_dataset = pd.read_csv(r"D:\TFG\datos\test3.csv", encoding='latin1')


# ### Análisis Inicial

# Mostramos las primeras lineas del dataset para ver la estructura y alguna información extra para ver si hay problemas previos

# In[7]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# In[8]:


dataset.describe()


# ### Preprocessing

# Vemos que hay muchas columnas con valores string, asi que hay que transformalos a ints para evitar futuros problemas a la hora de entrenar. Tambien creamos un diccionario con la relacion de las variables para poder identificarlas en un futuro

# In[9]:


variablesStr = ['Aula','SSID','BSSID']
label_mappings = {}

for variable in variablesStr:
    le = preprocessing.LabelEncoder()
    le.fit(dataset[variable])
    dataset[variable] = le.transform(dataset[variable])
    
    label_mappings[variable] = dict(zip(le.classes_, le.transform(le.classes_)))

#aplicamos la misma conversion al dataset test
for variable in variablesStr:
    le = preprocessing.LabelEncoder()
    le.classes_ = np.array(list(label_mappings[variable].keys()))  # Establecer clases según el mapeo

    # Transformar `test_dataset` usando el mapeo existente, si el valor no esta en el diccionario se pone a -1
    #al ser el dataset de test no deberia afectar que haya valores a -1
    test_dataset[variable] = test_dataset[variable].map(label_mappings[variable]).fillna(-1).astype(int)
    test2_dataset[variable] = test2_dataset[variable].map(label_mappings[variable]).fillna(-1).astype(int)
    test3_dataset[variable] = test3_dataset[variable].map(label_mappings[variable]).fillna(-1).astype(int)


# In[10]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# Ahora podemos ver por ejemplo, que el valor 0 pertenece a las redes wifi UAB y el 1 a las de eduroam

# In[11]:


print("Valores de 'SSID':")
for original, encoded in label_mappings['SSID'].items():
    print(f"{original} -> {encoded}")


# Ahora que ya tenemos un dataset con la información que queremos vamos a ver las correlaciones para ver que variables nos seran más útiles

# In[12]:


correlacio = dataset.corr()

plt.figure()

ax = sns.heatmap(correlacio, annot=True, linewidths=.5 ,annot_kws={'fontsize':5})


# Podemos ver que tenemos unas correlaciones bastante bajas, ya que la variable objetivo es Aula.

# En este caso, no nos va a hacer falta normalizar las variables, ya que no tienen unos valores muy dispares, a excepción del BSSID, pero esta variable ya esta correcta.

# In[13]:


dataset.describe()


# Para entrenar mejor la red neuronal vamos a transformar la estructura del dataset a un formato tabla, donde crearemos todas las cobinaciones de SSID y BSSID y guardaremos la media de la intensidad que reciben.

# In[14]:


copia_dataset = dataset
copia_dataset_test = test_dataset
copia_dataset_test2 = test2_dataset
copia_dataset_test3 = test3_dataset

copia_dataset['SSID_BSSID'] = copia_dataset['SSID'].astype(str) + "_" + copia_dataset['BSSID'].astype(str)
copia_dataset_test['SSID_BSSID'] = copia_dataset_test['SSID'].astype(str) + "_" + copia_dataset_test['BSSID'].astype(str)
copia_dataset_test2['SSID_BSSID'] = copia_dataset_test2['SSID'].astype(str) + "_" + copia_dataset_test2['BSSID'].astype(str)
copia_dataset_test3['SSID_BSSID'] = copia_dataset_test3['SSID'].astype(str) + "_" + copia_dataset_test3['BSSID'].astype(str)
# Crear la tabla pivotada
dataset_tabla = copia_dataset.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='mean').fillna(-200)
test_tabla = copia_dataset_test.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='mean').fillna(-200)
test2_tabla = copia_dataset_test2.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='mean').fillna(-200)
test3_tabla = copia_dataset_test3.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='mean').fillna(-200)
# Mostrar el DataFrame pivotado
print(dataset_tabla)
print("------------")
print(test_tabla)


# Si nos fijamos detenidamente vemos que hay columnas que no nos aparecen en el dataset de test, asi que añadiremos estas columnas para no tener problemas mas adelante.

# In[15]:


print("antes")
print(dataset_tabla.shape)
print(test_tabla.shape)
print(test2_tabla.shape)
print(test3_tabla.shape)

columnasTotales = dataset_tabla.columns
test_tabla = test_tabla.reindex(columns=columnasTotales, fill_value=-200)
test2_tabla = test2_tabla.reindex(columns=columnasTotales, fill_value=-200)
test3_tabla = test3_tabla.reindex(columns=columnasTotales, fill_value=-200)

print("después")
print(dataset_tabla.shape)
print(test_tabla.shape)
print(test2_tabla.shape)
print(test3_tabla.shape)


# Ahora que ya hay las mismas columnas, dividimos la variable objetivo del resto

# In[16]:


#separamos x de y
x = dataset_tabla.values
y = dataset_tabla.index

x_test = test_tabla.values
y_test = test_tabla.index

x_test2 = test2_tabla.values
y_test2 = test2_tabla.index

x_test3 = test3_tabla.values
y_test3 = test3_tabla.index

print(x_test[0])


# Entrenamos el modelo con keras

# In[17]:


#cambiamos los tipos para que no de error
#x = x.values if isinstance(x, pd.DataFrame) else x
#y = y.values if isinstance(y, pd.Series) else y

# Definir el modelo de red neuronal
model = Sequential([
    Dense(128, activation='relu', input_shape=(x.shape[1],)),
    Dense(64, activation='relu'),
    Dense(len(y), activation='softmax')  # Una salida por aula
])

# Compilar el modelo
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

#Entrenar el modelo sin validation_split, con early stopping loss, accuracy = 15-40
#early_stopping = EarlyStopping(monitor='loss', patience=5, restore_best_weights=True)
#history = model.fit(x, y, epochs=30, batch_size=4, callbacks=[early_stopping])

#Entrenar el modelo sin validation_split, con early stopping accuracy, accuracy = 30-50
#early_stopping = EarlyStopping(monitor='accuracy', patience=5, restore_best_weights=True)
#history = model.fit(x, y, epochs=30, batch_size=4, callbacks=[early_stopping])

#entrenar con validation data y val_loss. accuracy = 35-50
early_stopping = EarlyStopping(monitor='val_loss', patience=5, restore_best_weights=True)
history = model.fit(x, y, epochs=50, batch_size=4, validation_data=(x_test, y_test), callbacks=[early_stopping])

#entrenar con validation data y val_accuracy. accuracy = 20
#early_stopping = EarlyStopping(monitor='val_accuracy', patience=5, restore_best_weights=True)
#history = model.fit(x, y, epochs=50, batch_size=4, validation_data=(x_test, y_test), callbacks=[early_stopping])

#sin early stopping, accuracy = 
#history = model.fit(x, y, epochs=15, batch_size=4)


# Preparamos el test para aplicarlo a la red neuronal

# In[18]:


"""
loss, accuracy = model.evaluate(x_test, y_test)
print(f"Precisión en el conjunto de prueba: {accuracy * 100:.2f}%")

predicciones = model.predict(x_test)
predicciones_clases = np.argmax(predicciones, axis=1)

# Mostrar algunas predicciones vs etiquetas reales
for i in range(len(y_test)):  # Muestra las primeras 5 predicciones como ejemplo
    print(f"Predicción  test 1: {predicciones_clases[i]}, Real: {y_test[i]}")
  """  
#---------------------------------------------------------------------------------------
loss, accuracy = model.evaluate(x_test2, y_test2)
print(f"Precisión en el conjunto de prueba: {accuracy * 100:.2f}%")

predicciones = model.predict(x_test2)
predicciones_clases = np.argmax(predicciones, axis=1)

# Mostrar algunas predicciones vs etiquetas reales
for i in range(len(y_test2)):  # Muestra las primeras 5 predicciones como ejemplo
    print(f"Predicción  test 2: {predicciones_clases[i]}, Real: {y_test2[i]}")
    
#-----------------------------------------------------------------------------------------  
loss, accuracy = model.evaluate(x_test3, y_test3)
print(f"Precisión en el conjunto de prueba: {accuracy * 100:.2f}%")

predicciones = model.predict(x_test3)
predicciones_clases = np.argmax(predicciones, axis=1)

# Mostrar algunas predicciones vs etiquetas reales
for i in range(len(y_test3)):  # Muestra las primeras 5 predicciones como ejemplo
    print(f"Predicción  test 3: {predicciones_clases[i]}, Real: {y_test3[i]}")


# Probamos random forest

# In[22]:


from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, roc_curve, auc, classification_report, roc_auc_score

#entrenem les nostres dades
entrenamentRandomForest = RandomForestClassifier(n_estimators=150, random_state = 42)
entrenamentRandomForest.fit(x, y)

prediccio_test1 = entrenamentRandomForest.predict(x_test)
accuracyRandomForest = round(accuracy_score(y_test, prediccio_test1)*100,2)
print('accuracy test1:')
print(round(accuracyRandomForest,2), '%')
for i in range(len(y_test)):  # Muestra las predicciones
    print(f"Predicción  test: {prediccio_test1[i]}, Real: {y_test[i]}")


prediccio_test2 = entrenamentRandomForest.predict(x_test2)
accuracyRandomForest = round(accuracy_score(y_test2, prediccio_test2)*100,2)
print('accuracy test2:')
print(round(accuracyRandomForest,2), '%')
for i in range(len(y_test2)):  # Muestra las predicciones
    print(f"Predicción  test: {prediccio_test2[i]}, Real: {y_test2[i]}")

prediccio_test3 = entrenamentRandomForest.predict(x_test3)
accuracyRandomForest = round(accuracy_score(y_test3, prediccio_test3)*100,2)
print('accuracy test3:')
print(round(accuracyRandomForest,2), '%')
for i in range(len(y_test3)):  # Muestra las predicciones
    print(f"Predicción  test: {prediccio_test3[i]}, Real: {y_test3[i]}")


# In[23]:


import numpy as np
import matplotlib.pyplot as plt

# Inicializar listas para almacenar resultados
train_sizes = np.linspace(0.1, 1.0, 10)  # Proporciones del tamaño del entrenamiento
train_scores = []
test_scores = []

# Bucle para calcular accuracy con tamaños crecientes de entrenamiento
for size in train_sizes:
    # Determinar el subconjunto del entrenamiento
    n_samples = int(size * len(x))
    x_train_subset = x[:n_samples]
    y_train_subset = y[:n_samples]
    
    # Entrenar el modelo con el subconjunto
    model = RandomForestClassifier(n_estimators=150, random_state=42)
    model.fit(x_train_subset, y_train_subset)
    
    # Calcular accuracy en entrenamiento y prueba
    train_accuracy = model.score(x_train_subset, y_train_subset)
    test_accuracy = model.score(x_test, y_test)
    
    train_scores.append(train_accuracy)
    test_scores.append(test_accuracy)

# Graficar la curva de aprendizaje
plt.figure(figsize=(10, 6))
plt.plot(train_sizes * 100, train_scores, label='Precisión en entrenamiento', color='blue')
plt.plot(train_sizes * 100, test_scores, label='Precisión en prueba', color='orange')
plt.xlabel('Porcentaje del conjunto de entrenamiento utilizado')
plt.ylabel('Precisión (Accuracy)')
plt.title('Curva de aprendizaje del Random Forest')
plt.legend()
plt.grid(True)
plt.show()


# Como vemos que el random forest es lo mas efectivo lo transformamos a un modelo onxx para poder usarlo en nuestro proyecto de visual studio en c++

# ## EXPORTAR MODELO RANDOM FOREST

# In[24]:


#primero a documento pkl, tipo unico de python
import joblib
joblib.dump(model, 'random_forest_model.pkl')


# Primero transformamos el modelo a tipo onnx

# In[26]:


import skl2onnx
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType

#calculamos las features de nuestro random forest (el numero de wifis en nuestro caso)
n_features = entrenamentRandomForest.n_features_in_
print("Número de características del modelo:", n_features)
initial_type = [('float_input', FloatTensorType([None, n_features]))]

# Convertir el modelo a ONNX
onnx_model = convert_sklearn(entrenamentRandomForest, initial_types=initial_type)

# Guardar el modelo en un archivo ONNX
with open("random_forest_model.onnx3", "wb") as f:
    f.write(onnx_model.SerializeToString())
    
print("modelo exportado con exito")


# Ahora probamos que el modelo no ha perdido precision

# In[27]:


import onnxruntime as rt
import numpy as np

# Cargar el modelo ONNX
sess = rt.InferenceSession("random_forest_model3.onnx")

# Crear datos de prueba (asegúrate de que coincidan con la entrada del modelo)
x_test_onnx = x_test.astype(np.float32)

# Hacer predicciones
input_name = sess.get_inputs()[0].name
output_name = sess.get_outputs()[0].name
pred_onnx = sess.run([output_name], {input_name: x_test_onnx})

print("Predicción ONNX:", pred_onnx)

accuracy_onnx = round(accuracy_score(y_test, pred_onnx[0])*100,2)
print('accuracy onnx:')
print(round(accuracy_onnx,2), '%')


# Podemos ver que el modelo ha mantenido su buena accuracy

# ## EXPORTAR DICCIONARIOS

# In[29]:


import json

#primero Exportamos diccionario ssid
with open("diccionario_ssid.txt", "w") as file:
    for key, value in label_mappings["SSID"].items():
        file.write(f"{key}-{value}\n")
    
#ahora el bssid
with open("diccionario_bssid.txt", "w") as file:
    for key, value in label_mappings["BSSID"].items():
        file.write(f"{key}-{value}\n")

#diccionario aulas
with open("diccionario_aulas.txt", "w") as file:
    for key, value in label_mappings["Aula"].items():
        file.write(f"{key}:{value}\n")

# diccionario nombres columnas
with open("diccionario_nombres_columnas.txt","w") as file:
    for key in dataset_tabla.columns:
        file.write(f"{key}\n")
        
print("diccionarios exportados con exito")


# In[ ]:




