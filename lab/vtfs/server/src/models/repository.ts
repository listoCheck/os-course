import { QueryResult } from "pg";
import {dbPool} from "../db/db-pool";

type ColumnDef = {
    name: string;
    primary?: boolean;
    type?: string;
};

export class Repository {
    static tableName: string;
    static columns: ColumnDef[];

    static async findAll<T extends Repository>(): Promise<T[]> {
        const res: QueryResult = await dbPool.query(`SELECT * FROM ${this.tableName}`);
        return res.rows as T[];
    }

    static async findById<T extends Repository>(id: number): Promise<T | null> {
        const primaryColumn = this.columns.find(c => c.primary)?.name || "id";
        const res: QueryResult = await dbPool.query(
            `SELECT * FROM ${this.tableName} WHERE ${primaryColumn} = $1`,
            [id]
        );
        return res.rows[0] || null;
    }

    static async create<T extends Repository>(data: Record<string, any>): Promise<T> {
        const keys = Object.keys(data);
        const values = Object.values(data);
        const placeholders = keys.map((_, i) => `$${i + 1}`);
        const res: QueryResult = await dbPool.query(
            `INSERT INTO ${this.tableName} (${keys.join(", ")}) VALUES (${placeholders.join(", ")}) RETURNING *`,
            values
        );
        return res.rows[0] as T;
    }

    static async update<T extends Repository>(id: number, data: Record<string, any>): Promise<T | null> {
        const keys = Object.keys(data);
        const values = Object.values(data);
        const setString = keys.map((k, i) => `${k} = $${i + 1}`).join(", ");
        const primaryColumn = this.columns.find(c => c.primary)?.name || "id";

        const res: QueryResult = await dbPool.query(
            `UPDATE ${this.tableName} SET ${setString} WHERE ${primaryColumn} = $${keys.length + 1} RETURNING *`,
            [...values, id]
        );
        return res.rows[0] || null;
    }

    static async delete(id: number): Promise<boolean> {
        const primaryColumn = this.columns.find(c => c.primary)?.name || "id";
        const res: QueryResult = await dbPool.query(
            `DELETE FROM ${this.tableName} WHERE ${primaryColumn} = $1`,
            [id]
        );
        return res.rowCount > 0;
    }
}
