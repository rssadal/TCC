from flask import Flask, jsonify
import mysql.connector

app = Flask(__name__)

# Configuração do banco de dados MySQL
db_config = {
    'host': 'localhost',
    'user': 'root',
    'password': '12345678',
    'database': 'Estufa'
}

# Rota para consultar todas as plantas
@app.route('/plantas', methods=['GET'])
def consultar_plantas():
    try:
        # Conectar ao banco de dados
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor(dictionary=True)

        # Consultar todas as plantas
        cursor.execute("SELECT * FROM plantas")
        resultados = cursor.fetchall()

        # Fechar conexão
        cursor.close()
        conn.close()

        return jsonify(resultados), 200

    except Exception as e:
        return jsonify({'erro': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
