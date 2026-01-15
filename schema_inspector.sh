#!/bin/bash
# FSearch Database Schema Inspector
# This script helps you understand the structure of your FSearch database

FSEARCH_DB="$HOME/.local/share/fsearch/fsearch.db"

echo "======================================"
echo "FSearch Database Schema Inspector"
echo "======================================"
echo

# Check if database exists
if [ ! -f "$FSEARCH_DB" ]; then
    echo "❌ FSearch database not found at: $FSEARCH_DB"
    echo
    echo "Please:"
    echo "1. Make sure FSearch is installed"
    echo "2. Run FSearch and create a database (File → Update Database)"
    echo
    exit 1
fi

echo "✓ Found database at: $FSEARCH_DB"
echo "  Size: $(du -h "$FSEARCH_DB" | cut -f1)"
echo

# Check if sqlite3 is installed
if ! command -v sqlite3 &> /dev/null; then
    echo "❌ sqlite3 command not found"
    echo "Install it with: sudo pacman -S sqlite"
    exit 1
fi

echo "======================================"
echo "DATABASE SCHEMA"
echo "======================================"
echo

# Get schema
sqlite3 "$FSEARCH_DB" ".schema" | grep -v "^$"

echo
echo "======================================"
echo "TABLE INFORMATION"
echo "======================================"
echo

# List tables and their row counts
sqlite3 "$FSEARCH_DB" <<EOF
SELECT 'Table: ' || name || ' (' || (
    SELECT COUNT(*) FROM sqlite_master sm2 
    WHERE sm2.name = sqlite_master.name
) || ' rows)'
FROM sqlite_master 
WHERE type='table';
EOF

# Get detailed info for each table
TABLES=$(sqlite3 "$FSEARCH_DB" "SELECT name FROM sqlite_master WHERE type='table';")

for table in $TABLES; do
    echo
    echo "Table: $table"
    echo "----------------------------------------"
    
    # Get column info
    echo "Columns:"
    sqlite3 "$FSEARCH_DB" "PRAGMA table_info($table);" | awk -F'|' '{print "  - " $2 " (" $3 ")"}'
    
    # Get row count
    count=$(sqlite3 "$FSEARCH_DB" "SELECT COUNT(*) FROM $table;")
    echo "Row count: $count"
    
    # Show sample data
    if [ "$count" -gt 0 ]; then
        echo
        echo "Sample data (first 3 rows):"
        sqlite3 -header -column "$FSEARCH_DB" "SELECT * FROM $table LIMIT 3;"
    fi
done

echo
echo "======================================"
echo "SUGGESTED C++ CODE"
echo "======================================"
echo

# Generate suggested SQL query based on schema
if sqlite3 "$FSEARCH_DB" "SELECT name FROM sqlite_master WHERE type='table' AND name='files';" | grep -q "files"; then
    echo "Based on your database schema, use this SQL in fsearchrunner.cpp:"
    echo
    echo "const char *sql = "
    
    # Get actual column names
    COLUMNS=$(sqlite3 "$FSEARCH_DB" "PRAGMA table_info(files);" | cut -d'|' -f2 | tr '\n' ',' | sed 's/,$//')
    
    echo "    \"SELECT $COLUMNS FROM files \""
    echo "    \"WHERE name LIKE ?1 \""
    echo "    \"ORDER BY name COLLATE NOCASE \""
    echo "    \"LIMIT ?2\";"
    echo
    
    # Show how to access the columns
    echo "And access the data like this:"
    echo
    IFS=',' read -ra COLS <<< "$COLUMNS"
    idx=0
    for col in "${COLS[@]}"; do
        echo "const char *$col = reinterpret_cast<const char*>(sqlite3_column_text(stmt, $idx));"
        idx=$((idx + 1))
    done
fi

echo
echo "======================================"
echo "TEST QUERY"
echo "======================================"
echo

read -p "Enter a filename to search for (or press Enter to skip): " TEST_QUERY

if [ ! -z "$TEST_QUERY" ]; then
    echo
    echo "Searching for: $TEST_QUERY"
    echo
    
    sqlite3 -header -column "$FSEARCH_DB" <<EOF
SELECT * FROM files 
WHERE name LIKE '%$TEST_QUERY%' 
LIMIT 10;
EOF
fi

echo
echo "======================================"
echo "COMPLETE!"
echo "======================================"
echo
echo "Use this information to adjust the SQL queries in fsearchrunner.cpp"
echo "if the default schema doesn't match your database."
