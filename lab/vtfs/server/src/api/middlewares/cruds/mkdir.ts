import {Context, Next} from "koa";

export async function mkdir(ctx: Context, next: Next) {

    await next();
}