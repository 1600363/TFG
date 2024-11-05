#!/usr/bin/env python
# coding: utf-8

# # TFG

# In[1]:


import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn import preprocessing

import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense


# Importamos dataset

# In[2]:


dataset = pd.read_csv(r"D:\TFG\Bdwifis.csv", encoding='latin1')
test_dataset = pd.read_csv(r"D:\TFG\test.csv", encoding='latin1')


# ### Análisis Inicial

# Mostramos las primeras lineas del dataset para ver la estructura y alguna información extra para ver si hay problemas previos

# In[3]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# In[4]:


dataset.describe()


# ### Preprocessing

# Vemos que hay muchas columnas con valores string, asi que hay que transformalos a ints para evitar futuros problemas a la hora de entrenar. Tambien creamos un diccionario con la relacion de las variables para poder identificarlas en un futuro

# In[5]:


variablesStr = ['Aula','SSID','Hora','BSSID']
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


# In[6]:


pd.set_option('display.max_columns', None)
dataset.head(30)


# como queremos la información unicamente de la facultad, no tendremos en cuenta las wifis que no sean eduroam ni UAB, buscamos a que valores pertenecen esos casos y no tenemos en cuenta el resto

# In[7]:


print("Valores de 'SSID':")
for original, encoded in label_mappings['SSID'].items():
    print(f"{original} -> {encoded}")


# Vemos que los valores que pertenecen a eduroam y UAB son 21 y 17  respectivamente

# In[8]:


clean_dataset = dataset[(dataset['SSID'] == 21) | (dataset['SSID'] == 17)]
clean_test = test_dataset[(test_dataset['SSID'] == 21) | (test_dataset['SSID'] == 17)]


# In[9]:


pd.set_option('display.max_columns', None)
clean_dataset.head(30)


# Ahora que ya tenemos un dataset con la información que queremos vamos a ver las correlaciones para ver que variables nos seran más útiles

# In[10]:


correlacio = clean_dataset.corr()

plt.figure()

ax = sns.heatmap(correlacio, annot=True, linewidths=.5 ,annot_kws={'fontsize':5})


# Podemos ver que tenemos unas correlaciones bastante bajas, ya que la variable objetivo es Aula. Se observa también que la variable Hora y Posicion Aula no nos importan debido a su tan baja correlación y porque la hora y posición no afecta al aula donde estemos, asi que vamos a eliminarlas para que no causen problemas.

# In[11]:


clean_dataset = clean_dataset.drop(['Hora','Posición Aula'],axis = 1)
clean_test = clean_test.drop(['Hora','Posición Aula'],axis = 1)


# En este caso, no nos va a hacer falta normalizar las variables, ya que no tienen unos valores muy dispares, a excepción del BSSID, pero esta variable ya esta correcta.

# Finalmente la 

# In[12]:


dataset.describe()


# In[ ]:





# In[ ]:





# Para entrenar mejor la red neuronal vamos a transformar la estructura del dataset a un formato tabla, donde crearemos todas las cobinaciones de SSID y BSSID y guardaremos la media de la intensidad que reciben.

# In[13]:


copia_dataset = clean_dataset
copia_dataset2 = clean_test

copia_dataset['SSID_BSSID'] = copia_dataset['SSID'].astype(str) + "_" + copia_dataset['BSSID'].astype(str)
copia_dataset2['SSID_BSSID'] = copia_dataset2['SSID'].astype(str) + "_" + copia_dataset2['BSSID'].astype(str)
# Crear la tabla pivotada
dataset_tabla = copia_dataset.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='max').fillna(-200)
test_tabla = copia_dataset2.pivot_table(index='Aula', columns='SSID_BSSID', values='Intensity', aggfunc='max').fillna(-200)
# Mostrar el DataFrame pivotado
print(dataset_tabla)


# dividimos la variable objetivo del resto y comparamos los x_train con los x_test, y los y_train con y_test para asegurarnos que en caso falte alguna columna se añada

# In[14]:


#separamos x de y
x = dataset_tabla.values
y = dataset_tabla.index

x_test = test_tabla.values
y_test = test_tabla.index


#pasamos los valores a tipo dataframe
if isinstance(x, np.ndarray):
    x = pd.DataFrame(x, columns=[f'Feature{i}' for i in range(x.shape[1])])
if isinstance(x_test, np.ndarray):
    x_test = pd.DataFrame(x_test, columns=[f'Feature{i}' for i in range(x_test.shape[1])])

full_pivot_columns = pd.concat([x, x_test]).columns


# Reindexamos, si alguna columna no estixe se le pone valor -200, que es el peor valor posible
x = x.reindex(columns=full_pivot_columns, fill_value=-200)
x_test = x_test.reindex(columns=full_pivot_columns, fill_value=-200)


# Entrenamos el modelo con keras

# In[15]:


#cambiamos los tipos para que no de error
x = x.values if isinstance(x, pd.DataFrame) else x
y = y.values if isinstance(y, pd.Series) else y

# Definir el modelo de red neuronal
model = Sequential([
    Dense(128, activation='relu', input_shape=(x.shape[1],)),
    Dense(64, activation='relu'),
    Dense(len(le.classes_), activation='softmax')  # Una salida por aula
])

# Compilar el modelo
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

# Entrenar el modelo en todos los datos (sin validation_split)
history = model.fit(x, y, epochs=13, batch_size=4)


# Preparamos el test para aplicarlo a la red neuronal

# In[16]:


y_test = y_test.to_numpy() #runear solo 1 vez


# In[17]:


loss, accuracy = model.evaluate(x_test, y_test)
print(f"Precisión en el conjunto de prueba: {accuracy * 100:.2f}%")

predicciones = model.predict(x_test)
predicciones_clases = np.argmax(predicciones, axis=1)

# Mostrar algunas predicciones vs etiquetas reales
for i in range(4):  # Muestra las primeras 5 predicciones como ejemplo
    print(f"Predicción: {predicciones_clases[i]}, Real: {y_test[i]}")


# Probamos random forest

# In[27]:


from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, roc_curve, auc, classification_report, roc_auc_score, plot_confusion_matrix, confusion_matrix

#entrenem les nostres dades
entrenamentRandomForest = RandomForestClassifier(n_estimators=10000, random_state = 42)
entrenamentRandomForest.fit(x, y)
prediccioY = entrenamentRandomForest.predict(x_test)
accuracyRandomForest = round(accuracy_score(y_test, prediccioY)*100,2)
print('accuracy:')
print(round(accuracyRandomForest,2), '%')



# In[39]:


from sklearn.model_selection import train_test_split

xtrain, xtest,ytrain, ytest = train_test_split(x,y, test_size=0.2, random_state=42)
model = RandomForestClassifier(n_estimators=1çç0000, random_state=42)
model.fit(xtrain,ytrain)
ypred = model.predict(xtest)
acuracy = accuracy_score(ytest,ypred)
print(acuracy)

predictions = model.predict(x_test)
predictedaulas = le.inverse_transform(predictions)
acuracy2 = accuracy_score(predictions, y_test)
print(acuracy2)

for i in range(4):  # Muestra las primeras 5 predicciones como ejemplo
    print(f"Predicción: {predictions[i]}, Real: {y_test[i]}")


# In[ ]:





# In[ ]:





# In[ ]:




